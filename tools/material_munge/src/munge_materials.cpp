
#include "munge_materials.hpp"
#include "describe_material.hpp"
#include "material_flags.hpp"
#include "patch_material.hpp"
#include "req_file_helpers.hpp"
#include "string_utilities.hpp"
#include "synced_io.hpp"
#include "ucfb_tweaker.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include <boost/iostreams/device/mapped_file.hpp>
#include <gsl/gsl>

namespace sp {

using namespace std::literals;
namespace fs = std::filesystem;

namespace {

struct Hardcoded_material_flags {
   bool transparent = false;
   bool hard_edged = false;
   bool double_sided = false;
   bool statically_lit = false;
   bool unlit = false;
};

auto munge_material(const fs::path& material_path, const fs::path& output_file_path,
                    const std::unordered_map<Ci_string, YAML::Node>& descriptions)
   -> Hardcoded_material_flags
{
   auto root_node = YAML::LoadFile(material_path.string());

   if (!root_node["RenderType"s]) {
      throw std::runtime_error{"Null RenderType YAML node."s};
   }

   if (!root_node["Material"s]) {
      throw std::runtime_error{"Null Material YAML node."s};
   }

   if (!root_node["Textures"s]) {
      throw std::runtime_error{"Null Textures YAML node."s};
   }

   if (!root_node["Flags"s]) {
      throw std::runtime_error{"Null Flags YAML node."s};
   }

   const auto rendertype = root_node["RenderType"s].as<std::string>();

   const auto desc_name = make_ci_string(split_string_on(rendertype, "."sv)[0]);

   if (!descriptions.count(desc_name)) {
      throw std::runtime_error{"RenderType has no material description."s};
   }

   const auto material = describe_material(descriptions.at(desc_name), root_node);

   auto flags_node = root_node["Flags"s];

   Hardcoded_material_flags flags;

   flags.transparent = flags_node["Transparent"s].as<bool>(false);
   flags.hard_edged = flags_node["HardEdged"s].as<bool>(false);
   flags.double_sided = flags_node["DoubleSided"s].as<bool>(false);
   flags.statically_lit = flags_node["StaticallyLit"s].as<bool>(false);
   flags.unlit = flags_node["Unlit"s].as<bool>(false);

   std::vector<std::pair<std::string, std::vector<std::string>>> required_files;

   auto& required_sp_textures =
      required_files.emplace_back("sptex"s, std::vector<std::string>{}).second;

   for (const auto& texture : material.textures) {
      if (texture.empty() || texture.front() == '$') continue;

      required_sp_textures.emplace_back(texture);
   }

   write_patch_material(output_file_path, material);

   auto req_path = output_file_path;
   req_path.replace_extension(".texture.req"sv);

   emit_req_file(req_path, required_files);

   return flags;
}

void fixup_munged_model(const fs::path& model_path, Ci_String_view material_name,
                        Hardcoded_material_flags flags)
{
   using boost::iostreams::mapped_file;

   mapped_file file{model_path.u8string(), mapped_file::readwrite};

   auto file_span =
      gsl::make_span(reinterpret_cast<std::byte*>(file.data()), file.size());

   ucfb::Tweaker tweaker{file_span};

   auto modl_chunks = ucfb::find_all("modl"_mn, tweaker);

   for (auto& modl : modl_chunks) {
      auto segm_chunks = ucfb::find_all("segm"_mn, modl);

      for (auto& segm : segm_chunks) {
         auto mtrl_chunk = ucfb::find_next("MTRL"_mn, segm);
         auto tnam_chunks = ucfb::find_all("TNAM"_mn, segm);

         bool edit = false;

         for (auto& tnam : tnam_chunks) {
            tnam.get<std::int32_t>();

            auto string = tnam.read_string();

            edit = edit || (material_name == string);
         }

         if (!edit) continue;

         if (!mtrl_chunk) {
            throw std::runtime_error{"Segment in model did not have material info!"s};
         }

         auto& mtrl_flags = mtrl_chunk->get<Material_flags>();

         constexpr auto cleared_flags =
            Material_flags::glossmap | Material_flags::glow |
            Material_flags::perpixel | Material_flags::specular |
            Material_flags::env_map;

         mtrl_flags &= ~cleared_flags;

         // clang-format off

         if (flags.transparent) mtrl_flags |= Material_flags::transparent;
         else mtrl_flags &= ~Material_flags::transparent;

         if (flags.hard_edged) mtrl_flags |= Material_flags::hardedged;
         else mtrl_flags &= ~Material_flags::hardedged;

         if (flags.double_sided) mtrl_flags |= Material_flags::doublesided;
         else mtrl_flags &= ~Material_flags::doublesided;

         if (flags.statically_lit) mtrl_flags |= Material_flags::vertex_lit;
         else mtrl_flags &= ~Material_flags::vertex_lit;

         if (flags.unlit) mtrl_flags &= ~Material_flags::normal;
         else mtrl_flags |= Material_flags::normal;

         // clang-format on
      }
   }
}

void fixup_munged_models(
   const fs::path& output_dir,
   const std::unordered_map<Ci_string, std::vector<fs::path>>& texture_references,
   std::vector<std::pair<Ci_string, Hardcoded_material_flags>> munged_materials)
{
   for (const auto& [name, flags] : munged_materials) {
      if (!texture_references.count(name)) continue;

      for (const auto& file_ref : texture_references.at(name)) {
         auto output_file_path = output_dir / file_ref.filename();

         if (!fs::exists(output_file_path) ||
             (fs::last_write_time(output_file_path) < fs::last_write_time(file_ref))) {
            fs::copy_file(file_ref, output_file_path,
                          fs::copy_options::overwrite_existing);

            const auto extension = file_ref.extension() += ".req"sv;

            auto req_file_path = file_ref;
            req_file_path.replace_extension(extension);

            if (fs::exists(req_file_path)) {
               auto output_req_file_path = output_file_path;
               output_req_file_path.replace_extension(extension);

               fs::copy_file(req_file_path, output_req_file_path,
                             fs::copy_options::overwrite_existing);
            }
         }

         synced_print("Editing "sv, output_file_path.filename().string(),
                      " for material "sv, name, "..."sv);

         fixup_munged_model(output_file_path, name, flags);
      }
   }
}
}

void munge_materials(const fs::path& output_dir,
                     const std::unordered_map<Ci_string, std::vector<fs::path>>& texture_references,
                     const std::unordered_map<Ci_string, fs::path>& files,
                     const std::unordered_map<Ci_string, YAML::Node>& descriptions)
{
   std::vector<std::pair<Ci_string, Hardcoded_material_flags>> munged_materials;

   for (auto& file : files) {
      try {
         if (file.second.extension() == ".mtrl"s) {
            const auto output_file_path =
               output_dir / file.second.stem().replace_extension(".texture"s);

            if (fs::exists(output_file_path) &&
                (fs::last_write_time(file.second) <
                 fs::last_write_time(output_file_path))) {
               continue;
            }

            synced_print("Munging "sv, file.first, "..."sv);

            const auto flags =
               munge_material(file.second, output_file_path, descriptions);

            munged_materials.emplace_back(make_ci_string(file.second.stem().string()),
                                          flags);
         }
      }
      catch (std::exception& e) {
         synced_error_print("Error munging "sv, file.first, ": "sv, e.what());
      }
   }

   fixup_munged_models(output_dir, texture_references, munged_materials);
}
}
