
#include "user_config.hpp"

#include <filesystem>

namespace sp {

using namespace std::literals;

User_config user_config = User_config{R"(shader patch.yml)"s};

User_config::User_config(const std::string& path) noexcept
{
   using namespace std::literals;

   try {
      parse_file(path);
   }
   catch (std::exception& e) {
      log(Log_level::warning, "Failed to read config file "sv,
          std::quoted(path), ". reason:"sv, e.what());
   }
}

void User_config::show_imgui() noexcept
{
   ImGui::Begin("User Config", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

   if (ImGui::CollapsingHeader("Graphics", ImGuiTreeNodeFlags_DefaultOpen)) {
      graphics.antialiasing_method = aa_method_from_string(ImGui::StringPicker(
         "Anti-Aliasing Method", std::string{to_string(graphics.antialiasing_method)},
         std::initializer_list<std::string>{to_string(Antialiasing_method::none),
                                            to_string(Antialiasing_method::msaax2),
                                            to_string(Antialiasing_method::msaax4),
                                            to_string(Antialiasing_method::msaax8)}));

      graphics.anisotropic_filtering = anisotropic_filtering_from_string(ImGui::StringPicker(
         "Anisotropic Filtering", std::string{to_string(graphics.anisotropic_filtering)},
         std::initializer_list<std::string>{to_string(Anisotropic_filtering::off),
                                            to_string(Anisotropic_filtering::x2),
                                            to_string(Anisotropic_filtering::x4),
                                            to_string(Anisotropic_filtering::x8),
                                            to_string(Anisotropic_filtering::x16)}));
   }

   if (ImGui::CollapsingHeader("Effects", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Checkbox("Enabled", &effects.enabled);

      effects.color_quality = color_quality_from_string(ImGui::StringPicker(
         "Color Quality", std::string{to_string(effects.color_quality)},
         std::initializer_list<std::string>{to_string(Color_quality::normal),
                                            to_string(Color_quality::high),
                                            to_string(Color_quality::ultra)}));

      if (ImGui::IsItemHovered()) {
         ImGui::SetTooltip(
            "Does not have any effect when HDR Rendering is enabled.");
      }

      ImGui::Checkbox("Bloom", &effects.bloom);
      ImGui::Checkbox("Vignette", &effects.vignette);
      ImGui::Checkbox("Film Grain", &effects.film_grain);
      ImGui::Checkbox("Allow Colored Film Grain", &effects.colored_film_grain);
   }

   if (ImGui::CollapsingHeader("Developer")) {
      ImGui::Checkbox("Allow Event Queries", &developer.allow_event_queries);
   }

   ImGui::End();
}

void User_config::parse_file(const std::string& path)
{
   using namespace std::literals;

   const auto config = YAML::LoadFile(path);

   enabled = config["Shader Patch Enabled"s].as<bool>();

   display.screen_percent =
      std::clamp(config["Display"s]["Screen Percent"s].as<std::uint32_t>(), 10u, 100u);

   display.allow_tearing = config["Display"s]["Allow Tearing"s].as<bool>();

   display.centred = config["Display"s]["Centred"s].as<bool>();

   graphics.antialiasing_method = aa_method_from_string(
      config["Graphics"s]["Anti-Aliasing Method"s].as<std::string>());

   graphics.anisotropic_filtering = anisotropic_filtering_from_string(
      config["Graphics"s]["Anisotropic Filtering"s].as<std::string>());

   effects.enabled = config["Effects"s]["Enabled"s].as<bool>();

   effects.color_quality = color_quality_from_string(
      config["Effects"s]["Color Quality"].as<std::string>());

   effects.bloom = config["Effects"s]["Bloom"s].as<bool>();

   effects.vignette = config["Effects"s]["Vignette"s].as<bool>();

   effects.film_grain = config["Effects"s]["Film Grain"s].as<bool>();

   effects.colored_film_grain =
      config["Effects"s]["Allow Colored Film Grain"s].as<bool>();

   developer.toggle_key = config["Developer"s]["Screen Toggle"s].as<int>();

   developer.unlock_fps = config["Developer"s]["Unlock FPS"s].as<bool>();

   developer.allow_event_queries =
      config["Developer"s]["Allow Event Queries"s].as<bool>();

   developer.use_d3d11_debug_layer =
      config["Developer"s]["Use D3D11 Debug Layer"s].as<bool>();
}

}
