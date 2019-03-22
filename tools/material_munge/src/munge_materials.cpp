
#include "munge_materials.hpp"
#include "describe_material.hpp"
#include "file_helpers.hpp"
#include "material_options.hpp"
#include "model_patcher.hpp"
#include "patch_material_io.hpp"
#include "req_file_helpers.hpp"
#include "string_utilities.hpp"
#include "synced_io.hpp"

#include <array>
#include <cstdint>
#include <execution>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

#include <gsl/gsl>
#include <nlohmann/json.hpp>

namespace sp {

using namespace std::literals;
namespace fs = std::filesystem;

namespace {

auto load_materials_index(const fs::path& output_dir) noexcept
   -> std::pair<bool, std::unordered_map<Ci_string, Material_options>>
{
   const auto materials_index_path = output_dir / "materials_index.json"s;

   try {
      const auto file = load_string_file(materials_index_path);
      const auto index = nlohmann::json::parse(file);

      std::unordered_map<Ci_string, Material_options> result;

      for (const auto& entry : index.items()) {
         auto key = entry.key();

         result.emplace(make_ci_string(entry.key()),
                        entry.value().get<Material_options>());
      }

      // Remove the index file so in event of a crash a full munge will be triggered next time to clean up unruly files.
      fs::remove(materials_index_path);

      return {true, result};
   }
   catch (std::exception&) {
      return {false, {}};
   }
}

void save_materials_index(const fs::path& output_dir,
                          const std::unordered_map<Ci_string, Material_options>& material_index) noexcept
{
   const auto materials_index_path = output_dir / "materials_index.json"s;

   std::ofstream output{materials_index_path};

   nlohmann::json json;

   for (const auto& entry : material_index) {
      json[std::string{entry.first.begin(), entry.first.end()}] = entry.second;
   }

   output << json;
}

auto munge_material(const fs::path& material_path, const fs::path& output_file_path,
                    const std::unordered_map<Ci_string, YAML::Node>& descriptions)
   -> Material_options
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

   auto flags_node = root_node["Flags"s];

   Material_options options;

   options.transparent = flags_node["Transparent"s].as<bool>(false);
   options.hard_edged = flags_node["HardEdged"s].as<bool>(false);
   options.double_sided = flags_node["DoubleSided"s].as<bool>(false);
   options.statically_lit = flags_node["StaticallyLit"s].as<bool>(false);
   options.unlit = flags_node["Unlit"s].as<bool>(false);

   const auto rendertype = root_node["RenderType"s].as<std::string>();
   const auto desc_name = make_ci_string(split_string_on(rendertype, "."sv)[0]);

   if (!descriptions.count(desc_name)) {
      throw std::runtime_error{"RenderType has no material description."s};
   }

   const auto material =
      describe_material(material_path.stem().u8string(),
                        descriptions.at(desc_name), root_node, options);

   std::vector<std::pair<std::string, std::vector<std::string>>> required_files;

   const auto require_textures =
      [& required_sp_textures =
          required_files.emplace_back("sptex"s, std::vector<std::string>{}).second](
         const std::vector<std::string>& textures) {
         for (const auto& texture : textures) {
            if (texture.empty() || texture.front() == '$') continue;

            required_sp_textures.emplace_back(texture);
         }
      };

   require_textures(material.vs_textures);
   require_textures(material.hs_textures);
   require_textures(material.ds_textures);
   require_textures(material.gs_textures);
   require_textures(material.ps_textures);

   write_patch_material(output_file_path, material);

   auto req_path = output_file_path;
   req_path.replace_extension(".texture.req"sv);

   emit_req_file(req_path, required_files);

   return options;
}

void fixup_munged_models(
   const fs::path& output_dir,
   const std::unordered_map<Ci_string, std::vector<fs::path>>& texture_references,
   const std::unordered_map<Ci_string, Material_options>& material_index,
   const std::vector<Ci_string>& changed_materials)
{
   std::unordered_set<Ci_string> affected_models;
   std::mutex affected_models_mutex;

   std::for_each(std::execution::par, changed_materials.cbegin(),
                 changed_materials.cend(), [&](const Ci_string& name) noexcept {
                    auto tex_refs_it = texture_references.find(name);

                    if (tex_refs_it == texture_references.cend()) return;

                    std::for_each(
                       std::execution::par, tex_refs_it->second.cbegin(),
                       tex_refs_it->second.cend(), [&](const fs::path& file_ref) noexcept {
                          // Test if this model has already been processed.
                          {
                             std::lock_guard lock{affected_models_mutex};

                             if (auto [it, absent] = affected_models.emplace(
                                    make_ci_string(file_ref.filename().string()));
                                 !absent)
                                return;
                          }

                          auto output_file_path = output_dir / file_ref.filename();

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

                          synced_print("Editing "sv,
                                       output_file_path.filename().string(),
                                       " for Shader Patch..."sv);

                          patch_model(output_file_path, material_index);
                       });
                 });
}
}

void munge_materials(const fs::path& output_dir,
                     const std::unordered_map<Ci_string, std::vector<fs::path>>& texture_references,
                     const std::unordered_map<Ci_string, fs::path>& files,
                     const std::unordered_map<Ci_string, YAML::Node>& descriptions)
{
   std::vector<Ci_string> changed_materials;

   auto [partial_munge, index] = load_materials_index(output_dir);

   for (auto& file : files) {
      try {
         if (file.second.extension() != ".mtrl"_svci) continue;

         const auto output_file_path =
            output_dir / file.second.stem().replace_extension(".texture"s);

         if (partial_munge && fs::exists(output_file_path) &&
             (fs::last_write_time(file.second) < fs::last_write_time(output_file_path))) {
            continue;
         }

         synced_print("Munging "sv, file.first, "..."sv);

         const auto options =
            munge_material(file.second, output_file_path, descriptions);

         index[make_ci_string(file.second.stem().string())] = options;
         changed_materials.emplace_back(make_ci_string(file.second.stem().string()));
      }
      catch (std::exception& e) {
         synced_error_print("Error munging "sv, file.first, ": "sv, e.what());
      }
   }

   fixup_munged_models(output_dir, texture_references, index, changed_materials);
   save_materials_index(output_dir, index);
}
}
