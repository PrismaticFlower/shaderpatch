
#include "user_config.hpp"
#include "imgui/imgui_ext.hpp"
#include "shader_patch_version.hpp"

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

   if (ImGui::CollapsingHeader("User Interface", ImGuiTreeNodeFlags_DefaultOpen)) {
      const auto color_picker = [](const char* name, std::array<std::uint8_t, 3>& color) {
         std::array<float, 3> rgb_color{color[0] / 255.f, color[1] / 255.f,
                                        color[2] / 255.f};

         ImGui::ColorEdit3(name, rgb_color.data());

         color = {static_cast<std::uint8_t>(rgb_color[0] * 255),
                  static_cast<std::uint8_t>(rgb_color[1] * 255),
                  static_cast<std::uint8_t>(rgb_color[2] * 255)};
      };

      color_picker("Friend Color", ui.friend_color);
      color_picker("Foe Color", ui.foe_color);
   }

   if (ImGui::CollapsingHeader("Graphics", ImGuiTreeNodeFlags_DefaultOpen)) {
      graphics.antialiasing_method = aa_method_from_string(ImGui::StringPicker(
         "Anti-Aliasing Method", std::string{to_string(graphics.antialiasing_method)},
         std::initializer_list<std::string>{to_string(Antialiasing_method::none),
                                            to_string(Antialiasing_method::cmaa2),
                                            to_string(Antialiasing_method::msaax4),
                                            to_string(Antialiasing_method::msaax8)}));

      graphics.anisotropic_filtering = anisotropic_filtering_from_string(ImGui::StringPicker(
         "Anisotropic Filtering", std::string{to_string(graphics.anisotropic_filtering)},
         std::initializer_list<std::string>{to_string(Anisotropic_filtering::off),
                                            to_string(Anisotropic_filtering::x2),
                                            to_string(Anisotropic_filtering::x4),
                                            to_string(Anisotropic_filtering::x8),
                                            to_string(Anisotropic_filtering::x16)}));

      graphics.refraction_quality = refraction_quality_from_string(ImGui::StringPicker(
         "Refraction Quality", std::string{to_string(graphics.refraction_quality)},
         std::initializer_list<std::string>{to_string(Refraction_quality::low),
                                            to_string(Refraction_quality::medium),
                                            to_string(Refraction_quality::high),
                                            to_string(Refraction_quality::ultra)}));

      ImGui::Checkbox("Enable Order-Independent Transparency", &graphics.enable_oit);

      ImGui::Checkbox("Enable Alternative Post Processing",
                      &graphics.enable_alternative_postprocessing);

      ImGui::Checkbox("Enable Scene Blur", &graphics.enable_scene_blur);

      ImGui::Checkbox("Enable 16-Bit Color Channel Rendering",
                      &graphics.enable_16bit_color_rendering);

      ImGui::Checkbox("Disable Light Brightness Rescaling",
                      &graphics.disable_light_brightness_rescaling);

      ImGui::Checkbox("Enable User Effects Config", &graphics.enable_user_effects_config);
      ImGui::InputText("User Effects Config", graphics.user_effects_config);
   }

   if (ImGui::CollapsingHeader("Effects", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Checkbox("Bloom", &effects.bloom);
      ImGui::Checkbox("Vignette", &effects.vignette);
      ImGui::Checkbox("Film Grain", &effects.film_grain);
      ImGui::Checkbox("Allow Colored Film Grain", &effects.colored_film_grain);
      ImGui::Checkbox("SSAO", &effects.ssao);

      effects.ssao_quality = ssao_quality_from_string(ImGui::StringPicker(
         "SSAO Quality", std::string{to_string(effects.ssao_quality)},
         std::initializer_list<std::string>{to_string(SSAO_quality::fastest),
                                            to_string(SSAO_quality::fast),
                                            to_string(SSAO_quality::medium),
                                            to_string(SSAO_quality::high),
                                            to_string(SSAO_quality::highest)}));
   }

   ImGui::Text("Shader Patch v%s", current_shader_patch_version_string.c_str());

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

   display.treat_800x600_as_interface =
      config["Display"s]["Treat 800x600 As Interface"s].as<bool>();

   display.windowed_interface = config["Display"s]["Windowed Interface"s].as<bool>();

   display.dpi_aware = config["Display"s]["Display Scaling Aware"s].as<bool>();

   display.dpi_scaling = config["Display"s]["Display Scaling"s].as<bool>();

   display.scalable_fonts = config["Display"s]["Scalable Fonts"s].as<bool>();

   display.enable_game_perceived_resolution_override =
      config["Display"s]["Enable Game Perceived Resolution Override"s].as<bool>();

   display.game_perceived_resolution_override_width =
      config["Display"s]["Game Perceived Resolution Override"s][0].as<std::uint32_t>();

   display.game_perceived_resolution_override_height =
      config["Display"s]["Game Perceived Resolution Override"s][1].as<std::uint32_t>();

   for (std::size_t i = 0; i < 3; ++i) {
      ui.friend_color[i] =
       static_cast<std::uint8_t>(config["User Interface"s]["Friend Color"s][i].as<std::uint32_t>());
      ui.foe_color[i] =
        static_cast<std::uint8_t>(config["User Interface"s]["Foe Color"s][i].as<std::uint32_t>());
   }

   graphics.gpu_selection_method = gpu_selection_method_from_string(
      config["Graphics"s]["GPU Selection Method"s].as<std::string>());

   graphics.antialiasing_method = aa_method_from_string(
      config["Graphics"s]["Anti-Aliasing Method"s].as<std::string>());

   graphics.anisotropic_filtering = anisotropic_filtering_from_string(
      config["Graphics"s]["Anisotropic Filtering"s].as<std::string>());

   graphics.refraction_quality = refraction_quality_from_string(
      config["Graphics"s]["Refraction Quality"s].as<std::string>());

   graphics.enable_oit =
      config["Graphics"s]["Enable Order-Independent Transparency"s].as<bool>();

   graphics.enable_alternative_postprocessing =
      config["Graphics"s]["Enable Alternative Post Processing"s].as<bool>();

   graphics.enable_scene_blur = config["Graphics"s]["Enable Scene Blur"s].as<bool>();

   graphics.enable_16bit_color_rendering =
      config["Graphics"s]["Enable 16-Bit Color Channel Rendering"s].as<bool>();

   graphics.disable_light_brightness_rescaling =
      config["Graphics"s]["Disable Light Brightness Rescaling"s].as<bool>();

   graphics.enable_user_effects_config =
      config["Graphics"s]["Enable User Effects Config"s].as<bool>();

   graphics.user_effects_config =
      config["Graphics"s]["User Effects Config"s].as<std::string>();

   effects.bloom = config["Effects"s]["Bloom"s].as<bool>();

   effects.vignette = config["Effects"s]["Vignette"s].as<bool>();

   effects.film_grain = config["Effects"s]["Film Grain"s].as<bool>();

   effects.colored_film_grain =
      config["Effects"s]["Allow Colored Film Grain"s].as<bool>();

   effects.ssao = config["Effects"s]["SSAO"s].as<bool>();

   effects.ssao_quality = ssao_quality_from_string(
      config["Effects"s]["SSAO Quality"s].as<std::string>());

   developer.toggle_key = config["Developer"s]["Screen Toggle"s].as<int>();

   developer.monitor_bfront2_log =
      config["Developer"s]["Monitor BFront2.log"s].as<bool>();

   developer.allow_event_queries =
      config["Developer"s]["Allow Event Queries"s].as<bool>();

   developer.use_d3d11_debug_layer =
      config["Developer"s]["Use D3D11 Debug Layer"s].as<bool>();

   developer.use_dxgi_1_2_factory =
      config["Developer"s]["Use DXGI 1.2 Factory"s].as<bool>();

   developer.shader_cache_path =
      config["Developer"s]["Shader Cache Path"s].as<std::string>();

   developer.shader_definitions_path =
      config["Developer"s]["Shader Definitions Path"s].as<std::string>();

   developer.shader_source_path =
      config["Developer"s]["Shader Source Path"s].as<std::string>();

   developer.material_scripts_path =
      config["Developer"s]["Material Scripts Path"s].as<std::string>();

   developer.scalable_font_name =
      config["Developer"s]["Scalable Font Name"s].as<std::string>();
}

}
