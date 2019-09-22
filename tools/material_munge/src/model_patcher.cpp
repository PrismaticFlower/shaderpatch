
#include "model_patcher.hpp"
#include "compose_exception.hpp"
#include "generate_tangents.hpp"
#include "index_buffer.hpp"
#include "material_flags.hpp"
#include "memory_mapped_file.hpp"
#include "optimize_mesh.hpp"
#include "ucfb_editor.hpp"
#include "ucfb_tweaker.hpp"
#include "ucfb_writer.hpp"
#include "vertex_buffer.hpp"

#include <fstream>
#include <string>

#include <d3d9.h>

#pragma warning(disable : 4127) // conditional expression is constant, clean_vbufs triggers it for some reason

using namespace std::literals;

namespace sp {

namespace {

void clean_chunk(ucfb::Editor_parent_chunk& chunk) noexcept
{
   chunk.erase(std::remove_if(chunk.begin(), chunk.end(),
                              [](const auto& child) {
                                 const auto mn = child.first;

                                 return mn == "CSHD"_mn || mn == "VDAT"_mn ||
                                        mn == "SKIN"_mn;
                              }),
               chunk.end());
}

void clean_chunks(ucfb::Editor_parent_chunk& root) noexcept
{
   for (auto& chunk : root) {
      if (chunk.first != "modl"_mn) continue;
      if (std::holds_alternative<ucfb::Editor_data_chunk>(chunk.second))
         continue;

      clean_chunk(std::get<ucfb::Editor_parent_chunk>(chunk.second));
   }
}

void clean_vbufs(ucfb::Editor_parent_chunk& segm) noexcept
{
   Vbuf_flags ideal_vbuf{0u};

   for (auto it = ucfb::find(segm, "VBUF"_mn); it != segm.end();
        it = ucfb::find(it + 1, segm.end(), "VBUF"_mn)) {
      auto [count, stride, flags] =
         ucfb::Reader{it->first, std::get<ucfb::Editor_data_chunk>(it->second).span()}
            .read_multi<std::uint32_t, std::uint32_t, Vbuf_flags>();

      ideal_vbuf = std::max(ideal_vbuf, flags);
   }

   segm.erase(std::remove_if(segm.begin(), segm.end(),
                             [ideal_vbuf](const auto& child) {
                                if (child.first != "VBUF"_mn) return false;

                                auto [count, stride, flags] =
                                   ucfb::Reader{child.first,
                                                std::get<ucfb::Editor_data_chunk>(
                                                   child.second)
                                                   .span()}
                                      .read_multi<std::uint32_t, std::uint32_t, Vbuf_flags>();

                                return flags != ideal_vbuf;
                             }),
              segm.end());
}

void edit_ibuf_vbufs(ucfb::Editor_parent_chunk& segm, const Material_options options,
                     const std::array<glm::vec3, 2> vert_box)
{
   // Remove unused vertex buffers.
   clean_vbufs(segm);

   auto vbuf = ucfb::find(segm, "VBUF"_mn);

   auto index_buffer = create_index_buffer(
      ucfb::make_strict_reader<"IBUF"_mn>(ucfb::find(segm, "IBUF"_mn)));
   auto vertex_buffer =
      create_vertex_buffer(ucfb::make_strict_reader<"VBUF"_mn>(vbuf), vert_box);

   // Generate Tangents
   if (options.generate_tangents) {
      std::tie(index_buffer, vertex_buffer) =
         generate_tangents(index_buffer, vertex_buffer);
   }

   // Optimize triangle and vertex ordering.
   std::tie(index_buffer, vertex_buffer) = optimize_mesh(index_buffer, vertex_buffer);

   // Update IBUF.
   {
      auto& ibuf =
         std::get<ucfb::Editor_data_chunk>(ucfb::find(segm, "IBUF"_mn)->second);

      ibuf.clear();

      auto ibuf_writer = ibuf.writer();

      ibuf_writer.write(static_cast<std::uint32_t>(index_buffer.size() * 3),
                        gsl::as_bytes(gsl::make_span(index_buffer)));
   }

   // Update VBUF.
   {
      auto& vbuf_editor = std::get<ucfb::Editor_data_chunk>(vbuf->second);
      vbuf_editor.clear();

      auto vbuf_writer = vbuf_editor.writer();

      output_vertex_buffer(vertex_buffer, vbuf_writer, options.compressed, vert_box);
   }

   // Update INFO chunk.
   {
      auto info = ucfb::find(segm, "INFO"_mn);

      auto info_tweaker =
         ucfb::Tweaker{info->first,
                       std::get<ucfb::Editor_data_chunk>(info->second).span()};

      info_tweaker.get<std::uint32_t>().store(D3DPT_TRIANGLELIST);
      info_tweaker.get<std::uint32_t>().store(
         gsl::narrow_cast<std::uint32_t>(vertex_buffer.count));
      info_tweaker.get<std::uint32_t>().store(
         gsl::narrow_cast<std::uint32_t>(index_buffer.size()));
   }
}

void edit_mtrl(ucfb::Tweaker mtrl_tweaker, const Material_options options) noexcept
{
   auto mtrl_flags_proxy = mtrl_tweaker.get<Material_flags>();
   auto mtrl_flags = mtrl_flags_proxy.load();

   // Clear out flags that cause multiple render passes.
   constexpr auto cleared_flags =
      Material_flags::glossmap | Material_flags::glow | Material_flags::perpixel |
      Material_flags::specular | Material_flags::env_map;

   mtrl_flags &= ~cleared_flags;

   // clang-format off

   if (options.transparent) mtrl_flags |= Material_flags::transparent;
   else mtrl_flags &= ~Material_flags::transparent;

   if (options.hard_edged) mtrl_flags |= Material_flags::hardedged;
   else mtrl_flags &= ~Material_flags::hardedged;

   if (options.double_sided) mtrl_flags |= Material_flags::doublesided;
   else mtrl_flags &= ~Material_flags::doublesided;

   if (options.statically_lit) mtrl_flags |= Material_flags::vertex_lit;
   else mtrl_flags &= ~Material_flags::vertex_lit;

   if (options.unlit) mtrl_flags &= ~Material_flags::normal;
   else mtrl_flags |= Material_flags::normal;

   // clang-format on

   mtrl_flags_proxy.store(mtrl_flags);
}

void edit_tnams(ucfb::Editor_parent_chunk& segm) noexcept
{
   for (auto it = ucfb::find(segm, "TNAM"_mn); it != segm.end();
        it = ucfb::find(it + 1, segm.end(), "TNAM"_mn)) {
      const auto [index, texture] = [it] {
         auto reader = ucfb::make_reader(it);

         return std::pair{reader.read<std::uint32_t>(),
                          std::string{reader.read_string()}};
      }();

      auto& tanm = std::get<ucfb::Editor_data_chunk>(it->second);
      tanm.clear();

      auto writer = tanm.writer();

      if (index != 0)
         writer.write(index, ""sv);
      else
         writer.write(index, texture);
   }
}

void edit_segm(ucfb::Editor_parent_chunk& segm,
               const std::unordered_map<Ci_string, Material_options>& material_index,
               const std::array<glm::vec3, 2> vert_box)
{
   bool edit = false;

   Material_options options;

   for (auto tnam_it = ucfb::find(segm, "TNAM"_mn); tnam_it != segm.end();
        tnam_it = ucfb::find(tnam_it + 1, segm.end(), "TNAM"_mn)) {
      auto tnam =
         ucfb::Reader{tnam_it->first,
                      std::get<ucfb::Editor_data_chunk>(tnam_it->second).span()};
      const auto index = tnam.read<std::int32_t>();
      const auto string = tnam.read_string();

      if (auto it = material_index.find(make_ci_string(string));
          it == material_index.cend()) {
         continue;
      }
      else {
         options = it->second;
      }

      if (index != 0) {
         throw compose_exception<std::runtime_error>(
            "Segment uses material in a texture slot that isn't 0!"sv);
      }

      edit = true;
   }

   if (!edit) return;

   // Handle MTRL and RTYP
   auto mtrl_chunk = ucfb::find(segm, "MTRL"_mn);
   auto rtyp_chunk = ucfb::find(segm, "RTYP"_mn);

   if (mtrl_chunk == segm.end() || rtyp_chunk == segm.end()) {
      throw compose_exception<std::runtime_error>(
         "Segment in model did not have material info!"sv);
   }

   auto rtyp_reader =
      ucfb::Reader{rtyp_chunk->first,
                   std::get<ucfb::Editor_data_chunk>(rtyp_chunk->second).span()};

   if (rtyp_reader.read_string() != "Normal"sv) {
      throw compose_exception<std::runtime_error>(
         "Segment has RTYP that isn't \"Normal\"! This likely means you're attempting to apply a custom material to a refraction material."sv);
   }

   // Edit TNAM chunks to have only reference the material's texture.
   edit_tnams(segm);

   // Edit MTRL to have the correct render flags for use with custom materials.
   edit_mtrl({mtrl_chunk->first,
              std::get<ucfb::Editor_data_chunk>(mtrl_chunk->second).span()},
             options);

   // Edit IBUF and VBUFS to generate tangents and optimize face and vertex layout.
   edit_ibuf_vbufs(segm, options, vert_box);
}

void edit_modl(ucfb::Editor_parent_chunk& modl,
               const std::unordered_map<Ci_string, Material_options>& material_index)
{
   const auto vert_box = [&] {
      auto info = make_reader(ucfb::find(modl, "INFO"_mn));

      info.read_multi<std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t>();

      static_assert(sizeof(std::array<glm::vec3, 2>) == 24);

      return info.read<std::array<glm::vec3, 2>>();
   }();

   for (auto it = ucfb::find(modl, "segm"_mn); it != modl.end();
        it = ucfb::find(it + 1, modl.end(), "segm"_mn)) {
      edit_segm(std::get<ucfb::Editor_parent_chunk>(it->second), material_index,
                vert_box);
   }
}

void edit_modl_chunks(ucfb::Editor_parent_chunk& root,
                      const std::unordered_map<Ci_string, Material_options>& material_index)
{
   for (auto it = ucfb::find(root, "modl"_mn); it != root.end();
        it = ucfb::find(it + 1, root.end(), "modl"_mn)) {
      edit_modl(std::get<ucfb::Editor_parent_chunk>(it->second), material_index);
   }
}
}

void patch_model(const std::filesystem::path& model_path,
                 const std::filesystem::path& output_model_path,
                 const std::unordered_map<Ci_string, Material_options>& material_index)
{
   try {
      ucfb::Editor editor = [&] {
         win32::Memeory_mapped_file file{model_path,
                                         win32::Memeory_mapped_file::Mode::read};
         const auto is_parent = [](const Magic_number mn) noexcept
         {
            if (mn == "modl"_mn || mn == "shdw"_mn || mn == "segm"_mn)
               return true;

            return false;
         };

         return ucfb::Editor{ucfb::Reader_strict<"ucfb"_mn>{file.bytes()}, is_parent};
      }();

      // Strip out unused model chunks related to the fixed function pipeline.
      clean_chunks(editor);

      // Apply edits to `modl` chunks.
      edit_modl_chunks(editor, material_index);

      // Output new file.
      std::ofstream output{output_model_path, std::ios::binary};
      ucfb::Writer writer{output};
      editor.assemble(writer);
   }
   catch (std::exception& e) {
      throw compose_exception<std::runtime_error>(
         "Error occured while editing ", model_path, " ", e.what());
   }
}
}
