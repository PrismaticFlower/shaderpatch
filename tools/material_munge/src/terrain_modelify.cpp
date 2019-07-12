
#include "terrain_modelify.hpp"
#include "generate_tangents.hpp"
#include "index_buffer.hpp"
#include "material_flags.hpp"
#include "memory_mapped_file.hpp"
#include "swbf_fnv_1a.hpp"
#include "terrain_constants.hpp"
#include "terrain_model_segment.hpp"
#include "terrain_texture_transform.hpp"
#include "terrain_vertex_buffer.hpp"
#include "ucfb_editor.hpp"
#include "ucfb_writer.hpp"

#include <gsl/gsl>

#include <d3d9.h>

namespace fs = std::filesystem;
using namespace std::literals;

namespace sp {

namespace {

constexpr auto modl_max_segments = 48;

auto get_terrain_length(ucfb::Editor_parent_chunk& tern) -> float
{
   auto info = ucfb::make_strict_reader<"INFO"_mn>(ucfb::find(tern, "INFO"_mn));

   const auto [grid_scale, height_scale, min_height, max_height, terrain_size] =
      info.read_multi_unaligned<float, float, float, float, std::uint16_t>();

   return grid_scale * terrain_size;
}

auto get_terrain_vert_box(ucfb::Reader_strict<"INFO"_mn> info)
   -> std::array<glm::vec3, 2>
{
   const auto [grid_scale, height_scale, min_height, max_height, terrain_size] =
      info.read_multi_unaligned<float, float, float, float, std::uint16_t>();

   const float half_length = terrain_size / 2.0f * grid_scale;
   const float height_range_min =
      std::numeric_limits<std::uint16_t>::min() * height_scale;
   const float height_range_max =
      std::numeric_limits<std::uint16_t>::max() * height_scale;

   return {glm::vec3{-half_length, height_range_min, -half_length},
           glm::vec3{half_length, height_range_max, half_length}};
}

auto get_patch_texture_indices(ucfb::Reader_strict<"INFO"_mn> info)
   -> std::array<std::uint8_t, 3>
{
   const int count = std::clamp(info.read_unaligned<std::uint8_t>(),
                                std::uint8_t{0}, std::uint8_t{3});

   std::array<std::uint8_t, 3> indices{};

   for (auto i = 0; i < count; ++i) {
      indices[i] = info.read_unaligned<std::uint8_t>();
   }

   return indices;
}

void merge_in_patch(const Index_buffer_16& patch_indices,
                    const Terrain_vertex_buffer& vertices,
                    Terrain_triangle_list& out_triangle_list)
{
   for (const auto tri : patch_indices) {
      out_triangle_list.push_back(
         {vertices.at(tri[0]), vertices.at(tri[1]), vertices.at(tri[2])});
   }
}

auto get_tern_texture_transforms(const ucfb::Editor_parent_chunk& tern) noexcept
   -> std::array<Terrain_texture_transform, 16>
{
   const auto scales = ucfb::make_strict_reader<"SCAL"_mn>(ucfb::find(tern, "SCAL"_mn))
                          .read<std::array<float, 16>>();
   const auto axes = ucfb::make_strict_reader<"AXIS"_mn>(ucfb::find(tern, "AXIS"_mn))
                        .read<std::array<Terrain_texture_axis, 16>>();
   const auto rotations =
      ucfb::make_strict_reader<"ROTN"_mn>(ucfb::find(tern, "ROTN"_mn))
         .read<std::array<float, 16>>();

   std::array<Terrain_texture_transform, 16> transforms;

   for (auto i = 0; i < transforms.size(); ++i) {
      transforms[i].set_scale(scales[i]);
      transforms[i].set_axis(axes[i]);
      transforms[i].set_rotation(rotations[i]);
   }

   return transforms;
}

auto get_tern_geometry(const ucfb::Editor_parent_chunk& tern) -> Terrain_triangle_list
{
   const auto vert_box = get_terrain_vert_box(
      ucfb::make_strict_reader<"INFO"_mn>(ucfb::find(tern, "INFO"_mn)));

   auto pchs = ucfb::make_strict_reader<"PCHS"_mn>(ucfb::find(tern, "PCHS"_mn));

   const auto basic_index_buffer = create_index_buffer(
      pchs.read_child_strict<"COMN"_mn>().read_child_strict<"IBUF"_mn>());

   Terrain_triangle_list result_triangle_list;
   result_triangle_list.reserve(257 * 257);

   while (pchs) {
      auto ptch = pchs.read_child();
      if (ptch.magic_number() != "PTCH"_mn) continue;

      const auto texture_indices =
         get_patch_texture_indices(ptch.read_child_strict<"INFO"_mn>());
      const auto vertices =
         create_terrain_vertex_buffer(ptch.read_child_strict<"VBUF"_mn>(),
                                      texture_indices, vert_box);

      ptch.read_child_strict<"VBUF"_mn>(); // Uncompressed VBUF

      auto ibuf = ptch ? ptch.read_child_strict_optional<"IBUF"_mn>() : std::nullopt;
      auto custom_index_buffer =
         ibuf ? std::optional{create_index_buffer(*ibuf)} : std::nullopt;

      merge_in_patch(custom_index_buffer ? *custom_index_buffer : basic_index_buffer,
                     vertices, result_triangle_list);
   }

   return result_triangle_list;
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

void add_terrain_model_chunk(ucfb::Editor& editor, const int index,
                             const std::array<glm::vec3, 2> model_aabb,
                             const std::vector<Terrain_model_segment>& segments)
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
      auto& modl = std::get<1>(
         editor.emplace_back("modl"_mn, ucfb::Editor_parent_chunk{}).second);

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
         auto& segm = std::get<1>(
            modl.emplace_back("segm"_mn, ucfb::Editor_parent_chunk{}).second);

         // INFO
         {
            auto info =
               std::get<0>(
                  segm.emplace_back("INFO"_mn, ucfb::Editor_data_chunk{}).second)
                  .writer();

            info.write<std::uint32_t>(D3DPT_TRIANGLELIST); // primitive toplogy
            info.write(static_cast<std::uint32_t>(segment.vertices.size())); // vertex count
            info.write(static_cast<std::uint32_t>(segment.indices.size())); // primitive count
         }

         // MTRL
         {
            auto mtrl =
               std::get<0>(
                  segm.emplace_back("MTRL"_mn, ucfb::Editor_data_chunk{}).second)
                  .writer();

            mtrl.write(Material_flags::normal);    // material flags
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
               std::get<0>(
                  segm.emplace_back("RTYP"_mn, ucfb::Editor_data_chunk{}).second)
                  .writer();

            rtyp.write_unaligned("Normal"sv);
         }

         // TNAM 0
         {
            auto tnam =
               std::get<0>(
                  segm.emplace_back("TNAM"_mn, ucfb::Editor_data_chunk{}).second)
                  .writer();

            tnam.write<std::uint32_t>(0);
            tnam.write_unaligned(terrain_material_name);
         }

         // TNAM 1 2 3
         {
            for (auto i = 1; i < 4; ++i) {
               auto tnam =
                  std::get<0>(
                     segm.emplace_back("TNAM"_mn, ucfb::Editor_data_chunk{}).second)
                     .writer();

               tnam.write<std::uint32_t>(i);
               tnam.write_unaligned(""sv);
            }
         }

         // BBOX
         {
            auto bbox =
               std::get<0>(
                  segm.emplace_back("BBOX"_mn, ucfb::Editor_data_chunk{}).second)
                  .writer();

            static_assert(sizeof(segment.bbox) == 24);

            bbox.write(segment.bbox);
         }

         // IBUF
         {
            auto ibuf =
               std::get<0>(
                  segm.emplace_back("IBUF"_mn, ucfb::Editor_data_chunk{}).second)
                  .writer();

            ibuf.write(static_cast<std::uint32_t>(segment.indices.size() * 3));
            ibuf.write(gsl::as_bytes(gsl::make_span(segment.indices)));
         }

         // VBUF
         {
            auto vbuf =
               std::get<0>(
                  segm.emplace_back("VBUF"_mn, ucfb::Editor_data_chunk{}).second)
                  .writer();

            output_vertex_buffer(segment.vertices, vbuf, model_aabb);
         }

         // BNAM
         {
            auto bnam =
               std::get<0>(
                  segm.emplace_back("BNAM"_mn, ucfb::Editor_data_chunk{}).second)
                  .writer();

            bnam.write_unaligned("skel_bone_name"sv);
         }
      }

      // SPHR
      {
         auto sphr =
            std::get<0>(modl.emplace_back("SPHR"_mn, ucfb::Editor_data_chunk{}).second)
               .writer();

         sphr.write(glm::vec3{});
         sphr.write(glm::max(glm::length(model_aabb[0]), glm::length(model_aabb[1])));
      }
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

      // LOWD
      {
         auto lowd =
            std::get<0>(gmod.emplace_back("LOWD"_mn, ucfb::Editor_data_chunk{}).second)
               .writer();

         lowd.write_unaligned(model_name);
         lowd.write<std::uint32_t>(0); // vertex count
      }
   }
}

void add_terrain_model_chunks(ucfb::Editor& editor,
                              std::vector<Terrain_model_segment> segments)
{
   const auto model_aabb = calculate_terrain_model_segments_aabb(segments);
   const auto models = sort_terrain_segments_into_models(std::move(segments));

   for (auto i = 0; i < models.size(); ++i) {
      add_terrain_model_chunk(editor, i, model_aabb, models[i]);
   }

   // entc
   {
      auto& entc = std::get<1>(
         editor.emplace_back("entc"_mn, ucfb::Editor_parent_chunk{}).second);

      // BASE
      {
         auto base =
            std::get<0>(entc.emplace_back("BASE"_mn, ucfb::Editor_data_chunk{}).second)
               .writer();

         base.write_unaligned("prop"sv);
      }

      // TYPE
      {
         auto type =
            std::get<0>(entc.emplace_back("TYPE"_mn, ucfb::Editor_data_chunk{}).second)
               .writer();

         type.write_unaligned(terrain_object_name);
      }
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
      }
   }
}

void write_req_path(const std::filesystem::path& main_output_path)
{

   fs::path req_path{main_output_path};
   req_path += ".req"sv;

   std::ofstream file{req_path};
   file << R"(ucft
{
   REQN
   {
      "texture"
      "_SP_MATERIAL_terrain_geometry"
   }
})"sv;
}

}

void terrain_modelify(const std::filesystem::path& input_path,
                      const std::filesystem::path& output_path)
{
   Expects(fs::exists(input_path) && fs::is_regular_file(input_path) &&
           fs::exists(output_path.parent_path()));

   ucfb::Editor editor = [&] {
      win32::Memeory_mapped_file file{input_path,
                                      win32::Memeory_mapped_file::Mode::read};
      const auto is_parent = [](const Magic_number mn) noexcept {
         return mn == "tern"_mn;
      };

      return ucfb::Editor{ucfb::Reader_strict<"ucfb"_mn>{file.bytes()}, is_parent};
   }();

   auto& tern_editor =
      std::get<ucfb::Editor_parent_chunk>(ucfb::find(editor, "tern"_mn)->second);

   const auto terrain_texture_transforms = get_tern_texture_transforms(tern_editor);
   const auto terrain_triangle_list =
      generate_tangents(get_tern_geometry(tern_editor), terrain_texture_transforms);
   const auto terrain_model_segments = optimize_terrain_model_segments(
      create_terrain_model_segments(terrain_triangle_list,
                                    get_terrain_length(tern_editor)));

   remove_tern_geometry(tern_editor);

   add_terrain_model_chunks(editor, terrain_model_segments);

   auto file = ucfb::open_file_for_output(output_path);
   ucfb::Writer writer{file};
   editor.assemble(writer);

   write_req_path(output_path);
}
}
