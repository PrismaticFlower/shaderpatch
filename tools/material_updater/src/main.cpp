
#include "file_dialogs.hpp"
#include "glm_yaml_adapters.hpp"
#include "material_rendertype_property_mappings.hpp"
#include "string_utilities.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include <fmt/format.h>
#include <yaml-cpp/yaml.h>

#include <Windows.h>

using namespace std::literals;

constexpr auto exit_success = EXIT_SUCCESS;
constexpr auto exit_failure = EXIT_FAILURE;

void update_material_file(const std::filesystem::path& path);

auto format_value(const YAML::Node& value) -> std::string;

int main()
{
   std::ios_base::sync_with_stdio(false);
   CoInitializeEx(nullptr, COINIT_MULTITHREADED);

   auto target_folder =
      sp::win32::folder_dialog(nullptr, std::filesystem::current_path());

   if (!target_folder || !std::filesystem::exists(*target_folder)) {
      MessageBoxW(nullptr, L"You must select a valid target folder.",
                  L"Invalid Target", MB_OK);

      return exit_failure;
   }

   for (auto entry : std::filesystem::recursive_directory_iterator{
           *target_folder, std::filesystem::directory_options::skip_permission_denied}) {
      if (!entry.is_regular_file()) continue;
      if (entry.path().extension() != L".mtrl"sv) continue;

      std::wcout << L"Updating "sv << entry.path().native() << '\n';

      try {
         update_material_file(entry);
      }
      catch (std::exception&) {
         std::wcout << L"Failed to update "sv << entry.path().native() << '\n';
      }
   }

   MessageBoxW(nullptr, L"Done updating .mtrl files.", L"Done", MB_OK);
}

void update_material_file(const std::filesystem::path& path)
{
   auto mtrl = YAML::LoadFile(path.string());

   if (mtrl["Type"s]) return;

   if (mtrl["Flags"s]) mtrl.remove("Flags"s);

   auto rendertype = mtrl["RenderType"s].as<std::string>();
   auto root_type = sp::split_string_on(rendertype, "."sv)[0];

   if (root_type == "normal_ext-tessellated"sv) root_type = "normal_ext"sv;

   mtrl["Type"s] = std::string{root_type};
   mtrl.remove("RenderType"s);

   if (!sp::rendertype_property_mappings.contains(root_type)) return;

   auto properties = mtrl["Material"s];

   for (const auto& [str, prop] : sp::rendertype_property_mappings.at(root_type)) {
      if (!sp::contains(rendertype, str)) continue;

      std::visit([&](const auto& value) { properties[prop.name] = value.value; },
                 prop.value);
   }

   std::ofstream file{path};

   file << fmt::format("Type: {}"sv, mtrl["Type"s].as<std::string>());
   file << '\n';
   file << '\n';

   file << "Material:\n"sv;

   for (const auto& prop : mtrl["Material"s]) {
      file << fmt::format("   {}: {}"sv, prop.first.as<std::string>(),
                          format_value(prop.second));

      file << '\n';
      file << '\n';
   }

   file << "Textures:\n"sv;

   for (const auto& prop : mtrl["Textures"s]) {
      auto texture = prop.second.as<std::string>();

      file << fmt::format("   {}: {}"sv, prop.first.as<std::string>(),
                          texture.empty() ? R"("")"s : texture);

      file << '\n';
      file << '\n';
   }
}

auto format_value(const YAML::Node& value) -> std::string
{
   std::ostringstream stream;
   YAML::Emitter out{stream};

   out.SetBoolFormat(YAML::YesNoBool);
   out.SetSeqFormat(YAML::Flow);

   out << value;

   return stream.str();
}
