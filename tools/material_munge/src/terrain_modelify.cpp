
#include "terrain_modelify.hpp"
#include "generate_tangents.hpp"
#include "index_buffer.hpp"
#include "material_flags.hpp"
#include "memory_mapped_file.hpp"
#include "swbf_fnv_1a.hpp"
#include "terrain_constants.hpp"
#include "terrain_model_segment.hpp"
#include "terrain_texture_transform.hpp"
#include "ucfb_editor.hpp"
#include "ucfb_writer.hpp"

#include <iomanip>

#include <gsl/gsl>

#include <d3d9.h>

namespace fs = std::filesystem;
using namespace std::literals;

namespace sp {

namespace {

constexpr auto modl_max_segments = 48;

bool check_terrain(ucfb::Editor& editor) noexcept
{
   if (ucfb::find(editor, "tern"_mn) == editor.end()) return false;

   auto& tern =
      std::get<ucfb::Editor_parent_chunk>(ucfb::find(editor, "tern"_mn)->second);

   return ucfb::find(tern, "INFO"_mn) != tern.end() and
          ucfb::find(tern, "PCHS"_mn) != tern.end();
}

auto read_terrain_info(ucfb::Reader_strict<"INFO"_mn> reader) -> Terrain_info
{
   Terrain_info info{};

   info.grid_size = reader.read_unaligned<float>();
   info.height_scale = reader.read_unaligned<float>();
   info.height_min = reader.read_unaligned<float>();
   info.height_max = reader.read_unaligned<float>();
   info.terrain_length = reader.read_unaligned<std::uint16_t>();
   info.patch_length = reader.read_unaligned<std::uint16_t>();

   return info;
}

void remove_tern_geometry(ucfb::Editor_parent_chunk& tern)
{
   tern.erase(ucfb::find(tern, "PCHS"_mn));
}

auto sort_terrain_segments_into_models(std::vector<Terrain_model_segment> segments)
   -> std::vector<std::vector<Terrain_model_segment>>
{
   std::vector<std::vector<Terrain_model_segment>> result;
   result.reserve(terrain_max_objects);

   while (modl_max_segments <= segments.size()) {
      auto& model = result.emplace_back();

      for (auto i = 0; i < modl_max_segments; ++i) {
         model.emplace_back(std::move(segments.back()));
         segments.pop_back();
      }
   }

   if (!segments.empty()) {
      auto& model = result.emplace_back();

      for (auto& segment : segments) {
         model.emplace_back(std::move(segment));
      }
   }

   if (result.size() > terrain_max_objects) {
      throw std::runtime_error{"terrain requires uses too many objects!"};
   }

   return result;
}

void add_terrain_model_segm(ucfb::Editor_parent_chunk& modl,
                            const std::string_view material_name,
                            const std::string_view bone_name,
                            const Terrain_model_segment& segment,
                            const bool keep_static_lighting)
{
   auto& segm =
      std::get<1>(modl.emplace_back("segm"_mn, ucfb::Editor_parent_chunk{}).second);

   // INFO
   {
      auto info =
         std::get<0>(segm.emplace_back("INFO"_mn, ucfb::Editor_data_chunk{}).second)
            .writer();

      info.write<std::uint32_t>(D3DPT_TRIANGLELIST); // primitive toplogy
      info.write(static_cast<std::uint32_t>(segment.vertices.size())); // vertex count
      info.write(static_cast<std::uint32_t>(segment.indices.size())); // primitive count
   }

   // MTRL
   {
      auto mtrl =
         std::get<0>(segm.emplace_back("MTRL"_mn, ucfb::Editor_data_chunk{}).second)
            .writer();

      const auto flags = keep_static_lighting
                            ? Material_flags::normal | Material_flags::vertex_lit
                            : Material_flags::normal;

      mtrl.write(flags);                     // material flags
      mtrl.write<std::uint32_t>(0xffffffff); // diffuse color
      mtrl.write<std::uint32_t>(0xffffffff); // specular color
      mtrl.write<std::uint32_t>(50); // specular exponent (unused ingame)
      mtrl.write<std::uint32_t>(0);  // data0
      mtrl.write<std::uint32_t>(0);  // data1
      mtrl.write_unaligned(""sv);    // attached light name
   }

   // RTYP
   {
      auto rtyp =
         std::get<0>(segm.emplace_back("RTYP"_mn, ucfb::Editor_data_chunk{}).second)
            .writer();

      rtyp.write_unaligned("Normal"sv);
   }

   // TNAM 0
   {
      auto tnam =
         std::get<0>(segm.emplace_back("TNAM"_mn, ucfb::Editor_data_chunk{}).second)
            .writer();

      tnam.write<std::uint32_t>(0);
      tnam.write_unaligned(material_name);
   }

   // TNAM 1 2 3
   {
      for (auto i = 1; i < 4; ++i) {
         auto tnam =
            std::get<0>(segm.emplace_back("TNAM"_mn, ucfb::Editor_data_chunk{}).second)
               .writer();

         tnam.write<std::uint32_t>(i);
         tnam.write_unaligned(""sv);
      }
   }

   // BBOX
   {
      auto bbox =
         std::get<0>(segm.emplace_back("BBOX"_mn, ucfb::Editor_data_chunk{}).second)
            .writer();

      static_assert(sizeof(segment.bbox) == 24);

      bbox.write(segment.bbox);
   }

   // IBUF
   {
      auto ibuf =
         std::get<0>(segm.emplace_back("IBUF"_mn, ucfb::Editor_data_chunk{}).second)
            .writer();

      ibuf.write(static_cast<std::uint32_t>(segment.indices.size() * 3));
      ibuf.write(std::as_bytes(std::span{segment.indices}));
   }

   // VBUF
   {
      auto vbuf =
         std::get<0>(segm.emplace_back("VBUF"_mn, ucfb::Editor_data_chunk{}).second)
            .writer();

      vbuf.write(static_cast<std::uint32_t>(segment.vertices.size()),
                 static_cast<std::uint32_t>(sizeof(Terrain_vertex)),
                 keep_static_lighting ? terrain_vbuf_static_lighting_flags
                                      : terrain_vbuf_flags);
      vbuf.write(std::as_bytes(std::span{segment.vertices}));

      vbuf.write(std::uint64_t{}); // trailing unused texcoords & binormal
   }

   // BNAM
   {
      auto bnam =
         std::get<0>(segm.emplace_back("BNAM"_mn, ucfb::Editor_data_chunk{}).second)
            .writer();

      bnam.write_unaligned(bone_name);
   }
}

void add_terrain_model_modl(ucfb::Editor& editor, const std::string_view model_name,
                            const std::string_view skel_bone_name,
                            const std::string_view material_name,
                            const std::array<glm::vec3, 2> model_aabb,
                            const std::span<const Terrain_model_segment> segments,
                            const bool keep_static_lighting)
{
   auto& modl =
      std::get<1>(editor.emplace_back("modl"_mn, ucfb::Editor_parent_chunk{}).second);

   // NAME
   {
      auto name =
         std::get<0>(modl.emplace_back("NAME"_mn, ucfb::Editor_data_chunk{}).second)
            .writer();

      name.write_unaligned(model_name);
   }

   // VRTX
   {
      auto vrtx =
         std::get<0>(modl.emplace_back("VRTX"_mn, ucfb::Editor_data_chunk{}).second)
            .writer();

      vrtx.write<std::uint32_t>(0); // unknown
   }

   // NODE
   {
      auto node =
         std::get<0>(modl.emplace_back("NODE"_mn, ucfb::Editor_data_chunk{}).second)
            .writer();

      node.write_unaligned(skel_bone_name);
   }

   // INFO
   {
      auto info =
         std::get<0>(modl.emplace_back("INFO"_mn, ucfb::Editor_data_chunk{}).second)
            .writer();

      static_assert(sizeof(model_aabb) == 24);

      info.write<std::uint32_t>(0);                            // unknown
      info.write<std::uint32_t>(0);                            // unknown
      info.write(static_cast<std::uint32_t>(segments.size())); // segment count
      info.write<std::uint32_t>(0);                            // unknown
      info.write(model_aabb);
      info.write(model_aabb);
      info.write<std::uint32_t>(1); // unknown
      info.write<std::uint32_t>(0); // total vertex count, used for LOD and GPU budgeting
   }

   // segm(s)
   for (const auto& segment : segments) {
      add_terrain_model_segm(modl, material_name, skel_bone_name, segment,
                             keep_static_lighting);
   }

   // SPHR
   {
      auto sphr =
         std::get<0>(modl.emplace_back("SPHR"_mn, ucfb::Editor_data_chunk{}).second)
            .writer();

      sphr.write((model_aabb[0] + model_aabb[1]) / 2.0f);
      sphr.write(glm::max(glm::length(model_aabb[0]), glm::length(model_aabb[1])));
   }
}

void add_terrain_model_chunk(ucfb::Editor& editor,
                             const std::string_view material_name, const int index,
                             const std::array<glm::vec3, 2> model_aabb,
                             const std::vector<Terrain_model_segment>& segments,
                             const bool keep_static_lighting)
{
   constexpr std::string_view skel_bone_name = "root";
   const auto model_name = std::string{terrain_model_name} + std::to_string(index);

   // skel
   {
      auto& skel = std::get<1>(
         editor.emplace_back("skel"_mn, ucfb::Editor_parent_chunk{}).second);

      // INFO
      {
         auto info =
            std::get<0>(skel.emplace_back("INFO"_mn, ucfb::Editor_data_chunk{}).second)
               .writer();

         info.write_unaligned(model_name);
         info.write_unaligned(std::uint16_t{1});
      }

      // NAME
      {
         auto name =
            std::get<0>(skel.emplace_back("NAME"_mn, ucfb::Editor_data_chunk{}).second)
               .writer();

         name.write_unaligned(skel_bone_name);
      }

      // PRNT
      {
         auto prnt =
            std::get<0>(skel.emplace_back("PRNT"_mn, ucfb::Editor_data_chunk{}).second)
               .writer();

         prnt.write_unaligned(""sv);
      }

      // XFRM
      {
         auto xfrm =
            std::get<0>(skel.emplace_back("XFRM"_mn, ucfb::Editor_data_chunk{}).second)
               .writer();

         xfrm.write(std::array{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                               1.0f, 0.0f, 0.0f, 0.0f});
      }
   }

   // modl
   {
      add_terrain_model_modl(editor, model_name, skel_bone_name, material_name,
                             model_aabb, segments, keep_static_lighting);
   }

   // gmod
   {
      auto& gmod = std::get<1>(
         editor.emplace_back("gmod"_mn, ucfb::Editor_parent_chunk{}).second);

      // NAME
      {
         auto name =
            std::get<0>(gmod.emplace_back("NAME"_mn, ucfb::Editor_data_chunk{}).second)
               .writer();

         name.write_unaligned(model_name);
      }

      // INFO
      {
         auto info =
            std::get<0>(gmod.emplace_back("INFO"_mn, ucfb::Editor_data_chunk{}).second)
               .writer();

         info.write<std::uint32_t, float, std::uint32_t>(0, 1.0f, 0);
      }

      // LOD0
      {
         auto lod0 =
            std::get<0>(gmod.emplace_back("LOD0"_mn, ucfb::Editor_data_chunk{}).second)
               .writer();

         lod0.write_unaligned(model_name);
         lod0.write<std::uint32_t>(0); // vertex count
      }

      {
         // LOWD
         auto lowd =
            std::get<0>(gmod.emplace_back("LOWD"_mn, ucfb::Editor_data_chunk{}).second)
               .writer();

         lowd.write_unaligned(model_name);
         lowd.write<std::uint32_t>(0); // vertex count
      }
   }
}

void add_terrain_model_chunks(ucfb::Editor& editor, const std::string_view material_name,
                              const std::vector<Terrain_model_segment>& segments,
                              const bool keep_static_lighting)
{
   const auto model_aabb = calculate_terrain_model_segments_aabb(segments);
   const auto models = sort_terrain_segments_into_models(std::move(segments));

   for (auto i = 0; i < models.size(); ++i) {
      add_terrain_model_chunk(editor, material_name, i, model_aabb, models[i],
                              keep_static_lighting);
   }

   // wrld
   {
      auto& wrld = std::get<1>(
         editor.emplace_back("wrld"_mn, ucfb::Editor_parent_chunk{}).second);

      // NAME
      {
         auto name =
            std::get<0>(wrld.emplace_back("NAME"_mn, ucfb::Editor_data_chunk{}).second)
               .writer();

         name.write_unaligned(terrain_world_name);
      }

      // INFO
      {
         auto info =
            std::get<0>(wrld.emplace_back("INFO"_mn, ucfb::Editor_data_chunk{}).second)
               .writer();

         info.write<std::uint32_t>(0);                          // unknown
         info.write(static_cast<std::uint32_t>(models.size())); // instance count
      }

      // inst
      for (auto i = 0; i < models.size(); ++i) {
         auto& inst = std::get<1>(
            wrld.emplace_back("inst"_mn, ucfb::Editor_parent_chunk{}).second);

         // INFO
         {
            auto& info = std::get<1>(
               inst.emplace_back("INFO"_mn, ucfb::Editor_parent_chunk{}).second);

            // TYPE
            {
               auto type =
                  std::get<0>(
                     info.emplace_back("TYPE"_mn, ucfb::Editor_data_chunk{}).second)
                     .writer();

               type.write_unaligned("com_inv_col_8"sv);
            }

            // NAME
            {
               auto name =
                  std::get<0>(
                     info.emplace_back("NAME"_mn, ucfb::Editor_data_chunk{}).second)
                     .writer();

               name.write_unaligned(""sv);
            }

            // XFRM
            {
               auto xfrm =
                  std::get<0>(
                     info.emplace_back("XFRM"_mn, ucfb::Editor_data_chunk{}).second)
                     .writer();

               xfrm.write(std::array{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                                     0.0f, 1.0f, 0.0f, 0.0f, 0.0f});
            }
         }

         // PROP
         {
            auto prop =
               std::get<0>(
                  inst.emplace_back("PROP"_mn, ucfb::Editor_data_chunk{}).second)
                  .writer();

            const auto model_name =
               std::string{terrain_model_name} + std::to_string(i);

            prop.write_unaligned("GeometryName"_fnv, model_name);
         }

         // PROP
         {
            auto prop =
               std::get<0>(
                  inst.emplace_back("PROP"_mn, ucfb::Editor_data_chunk{}).second)
                  .writer();

            prop.write_unaligned("IsCollidable"_fnv, "0"sv);
         }
      }
   }
}

void write_req_path(const std::filesystem::path& main_output_path,
                    const std::string_view material_name)
{

   fs::path req_path{main_output_path};
   req_path += ".req"sv;

   std::ofstream file{req_path};
   file << R"(ucft
{
   REQN
   {
      "material"
)"sv << std::quoted(material_name)
        << R"(
   }
})"sv;
}
}

void terrain_modelify(const std::string_view material_suffix,
                      const bool keep_static_lighting,
                      const std::filesystem::path& munged_input_terrain_path,
                      const std::filesystem::path& output_path)
{
   Expects(fs::exists(munged_input_terrain_path) &&
           fs::is_regular_file(munged_input_terrain_path) &&
           fs::exists(output_path.parent_path()));

   ucfb::Editor editor = [&] {
      win32::Memeory_mapped_file file{munged_input_terrain_path,
                                      win32::Memeory_mapped_file::Mode::read};
      const auto is_parent = [](const Magic_number mn) noexcept {
         return mn == "tern"_mn;
      };

      return ucfb::Editor{ucfb::Reader_strict<"ucfb"_mn>{file.bytes()}, is_parent};
   }();

   if (!check_terrain(editor)) {
      throw std::runtime_error{
         "Bad munged terrain. Missing tern, INFO or PCHS chunk. This is quite "
         "sad "
         "news so have a sad face =( But no hints on how to solve it because "
         "we don't know! Try cleaning and making sure your terrain is enabled "
         "maybe? Good luck!"};
   }

   auto& tern_editor =
      std::get<ucfb::Editor_parent_chunk>(ucfb::find(editor, "tern"_mn)->second);

   const auto terrain_model_segments =
      create_terrain_model_segments(ucfb::make_strict_reader<"PCHS"_mn>(
                                       ucfb::find(tern_editor, "PCHS"_mn)),
                                    read_terrain_info(ucfb::make_strict_reader<"INFO"_mn>(
                                       ucfb::find(tern_editor, "INFO"_mn))));

   std::string material_name{terrain_material_name};
   material_name += material_suffix;

   remove_tern_geometry(tern_editor);

   add_terrain_model_chunks(editor, material_name, terrain_model_segments,
                            keep_static_lighting);

   auto file = ucfb::open_file_for_output(output_path);
   ucfb::File_writer writer{"ucfb"_mn, file};
   editor.assemble(writer);

   write_req_path(output_path, material_name);
}
}
