
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

   if (ImGui::CollapsingHeader("Display", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Checkbox("V-Sync", &display.v_sync);
   }

   if (ImGui::CollapsingHeader("Graphics", ImGuiTreeNodeFlags_DefaultOpen)) {
      graphics.antialiasing_method = aa_method_from_string(ImGui::StringPicker(
         "Anti-Aliasing Method", std::string{to_string(graphics.antialiasing_method)},
         std::initializer_list<std::string>{to_string(Antialiasing_method::none),
                                            to_string(Antialiasing_method::cmaa2),
                                            to_string(Antialiasing_method::msaax4),
                                            to_string(Antialiasing_method::msaax8)}));

      ImGui::Checkbox("Supersample Alpha Test", &graphics.supersample_alpha_test);

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

      ImGui::Checkbox("Allow Vertex Soft Skinning", &graphics.allow_vertex_soft_skinning);

      ImGui::Checkbox("Enable Scene Blur", &graphics.enable_scene_blur);

      ImGui::Checkbox("Enable 16-Bit Color Channel Rendering",
                      &graphics.enable_16bit_color_rendering);

      ImGui::Checkbox("Disable Light Brightness Rescaling",
                      &graphics.disable_light_brightness_rescaling);

      ImGui::Checkbox("Enable User Effects Config", &graphics.enable_user_effects_config);
      ImGui::InputText("User Effects Config", graphics.user_effects_config);
      ImGui::Checkbox("Enable Auto User Effects Config",
                      &graphics.enable_user_effects_auto_config);
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

      ImGui::Checkbox("Depth of Field", &effects.ssao);

      effects.dof_quality = dof_quality_from_string(ImGui::StringPicker(
         "Depth of Field Quality", std::string{to_string(effects.dof_quality)},
         std::initializer_list<std::string>{to_string(DOF_quality::ultra_performance),
                                            to_string(DOF_quality::performance),
                                            to_string(DOF_quality::quality),
                                            to_string(DOF_quality::ultra_quality)}));
   }

   ImGui::Text("Shader Patch v%s", current_shader_patch_version_string.c_str());

   ImGui::End();
}

void User_config::parse_file(const std::string& path)
{
   using namespace std::literals;

   const auto config = YAML::LoadFile(path);

   enabled = config["Shader Patch Enabled"s].as<bool>(enabled);

   display.v_sync = config["Display"s]["V-Sync"s].as<bool>(display.v_sync);

   display.treat_800x600_as_interface =
      config["Display"s]["Treat 800x600 As Interface"s].as<bool>(
         display.treat_800x600_as_interface);

   display.stretch_interface =
      config["Display"s]["Stretch Interface"s].as<bool>(display.stretch_interface);

   display.dpi_aware =
      config["Display"s]["Display Scaling Aware"s].as<bool>(display.dpi_aware);

   display.dpi_scaling =
      config["Display"s]["Display Scaling"s].as<bool>(display.dpi_scaling);

   display.dsr_vsr_scaling =
      config["Display"s]["DSR-VSR Display Scaling"s].as<bool>(display.dsr_vsr_scaling);

   display.scalable_fonts =
      config["Display"s]["Scalable Fonts"s].as<bool>(display.scalable_fonts);

   display.enable_game_perceived_resolution_override =
      config["Display"s]["Enable Game Perceived Resolution Override"s].as<bool>(
         display.enable_game_perceived_resolution_override);

   display.game_perceived_resolution_override_width =
      config["Display"s]["Game Perceived Resolution Override"s][0].as<std::uint32_t>(
         display.game_perceived_resolution_override_width);

   display.game_perceived_resolution_override_height =
      config["Display"s]["Game Perceived Resolution Override"s][1].as<std::uint32_t>(
         display.game_perceived_resolution_override_height);

   display.aspect_ratio_hack =
      config["Display"s]["Aspect Ratio Hack"s].as<bool>(display.aspect_ratio_hack);

   display.aspect_ratio_hack_hud = aspect_ratio_hud_from_string(
      config["Display"s]["Aspect Ratio Hack HUD Handling"s].as<std::string>(
         to_string(display.aspect_ratio_hack_hud)));

   display.override_resolution =
      config["Display"s]["Override Resolution"s].as<bool>(display.override_resolution);

   display.override_resolution_screen_percent =
      std::clamp(config["Display"s]["Override Resolution Screen Percent"s].as<std::uint32_t>(
                    display.override_resolution_screen_percent),
                 50u, 100u);

   ui.extra_ui_scaling =
      std::clamp(config["User Interface"s]["Extra UI Scaling"s].as<std::uint32_t>(
                    ui.extra_ui_scaling),
                 100u, 200u);

   for (std::size_t i = 0; i < 3; ++i) {
      ui.friend_color[i] = static_cast<std::uint8_t>(
         config["User Interface"s]["Friend Color"s][i].as<std::uint32_t>(
            ui.friend_color[i]));
      ui.foe_color[i] = static_cast<std::uint8_t>(
         config["User Interface"s]["Foe Color"s][i].as<std::uint32_t>(ui.foe_color[i]));
   }

   graphics.gpu_selection_method = gpu_selection_method_from_string(
      config["Graphics"s]["GPU Selection Method"s].as<std::string>("Highest Performance"s));

   graphics.antialiasing_method = aa_method_from_string(
      config["Graphics"s]["Anti-Aliasing Method"s].as<std::string>(
         to_string(graphics.antialiasing_method)));

   graphics.supersample_alpha_test =
      config["Graphics"s]["Supersample Alpha Test"s].as<bool>();

   graphics.anisotropic_filtering = anisotropic_filtering_from_string(
      config["Graphics"s]["Anisotropic Filtering"s].as<std::string>(
         to_string(graphics.anisotropic_filtering)));

   graphics.refraction_quality = refraction_quality_from_string(
      config["Graphics"s]["Refraction Quality"s].as<std::string>(
         to_string(graphics.refraction_quality)));

   graphics.enable_oit =
      config["Graphics"s]["Enable Order-Independent Transparency"s].as<bool>(
         graphics.enable_oit);

   graphics.enable_alternative_postprocessing =
      config["Graphics"s]["Enable Alternative Post Processing"s].as<bool>(
         graphics.enable_alternative_postprocessing);

   graphics.allow_vertex_soft_skinning =
      config["Graphics"s]["Allow Vertex Soft Skinning"s].as<bool>(
         graphics.allow_vertex_soft_skinning);

   graphics.enable_scene_blur =
      config["Graphics"s]["Enable Scene Blur"s].as<bool>(graphics.enable_scene_blur);

   graphics.enable_16bit_color_rendering =
      config["Graphics"s]["Enable 16-Bit Color Channel Rendering"s].as<bool>(
         graphics.enable_16bit_color_rendering);

   graphics.disable_light_brightness_rescaling =
      config["Graphics"s]["Disable Light Brightness Rescaling"s].as<bool>(
         graphics.disable_light_brightness_rescaling);

   graphics.enable_user_effects_config =
      config["Graphics"s]["Enable User Effects Config"s].as<bool>(
         graphics.enable_user_effects_config);

   graphics.user_effects_config =
      config["Graphics"s]["User Effects Config"s].as<std::string>(
         graphics.user_effects_config);

   graphics.enable_user_effects_auto_config =
      config["Graphics"s]["Enable Auto User Effects Config"s].as<bool>(
         graphics.enable_user_effects_auto_config);

   graphics.use_d3d11on12 =
      config["Graphics"s]["Use Direct3D 11 on 12"s].as<bool>(graphics.use_d3d11on12);

   effects.bloom = config["Effects"s]["Bloom"s].as<bool>(effects.bloom);

   effects.vignette = config["Effects"s]["Vignette"s].as<bool>(effects.vignette);

   effects.film_grain = config["Effects"s]["Film Grain"s].as<bool>(effects.film_grain);

   effects.colored_film_grain =
      config["Effects"s]["Allow Colored Film Grain"s].as<bool>(effects.colored_film_grain);

   effects.ssao = config["Effects"s]["SSAO"s].as<bool>(effects.ssao);

   effects.ssao_quality =
      ssao_quality_from_string(config["Effects"s]["SSAO Quality"s].as<std::string>(
         to_string(effects.ssao_quality)));

   effects.dof = config["Effects"s]["Depth of Field"s].as<bool>(effects.dof);

   effects.dof_quality = dof_quality_from_string(
      config["Effects"s]["Depth of Field Quality"s].as<std::string>(
         to_string(effects.dof_quality)));

   developer.toggle_key =
      config["Developer"s]["Screen Toggle"s].as<int>(developer.toggle_key);

   developer.monitor_bfront2_log =
      config["Developer"s]["Monitor BFront2.log"s].as<bool>(developer.monitor_bfront2_log);

   developer.allow_event_queries =
      config["Developer"s]["Allow Event Queries"s].as<bool>(developer.allow_event_queries);

   developer.use_d3d11_debug_layer =
      config["Developer"s]["Use D3D11 Debug Layer"s].as<bool>(
         developer.use_d3d11_debug_layer);

   developer.use_dxgi_1_2_factory =
      config["Developer"s]["Use DXGI 1.2 Factory"s].as<bool>(developer.use_dxgi_1_2_factory);

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

auto gpu_selection_method_from_string(const std::string_view string) noexcept
   -> GPU_selection_method
{
   if (string == "Highest Performance") {
      return GPU_selection_method::highest_performance;
   }
   else if (string == "Lowest Power Usage") {
      return GPU_selection_method::lowest_power_usage;
   }
   else if (string == "Highest Feature Level") {
      return GPU_selection_method::highest_feature_level;
   }
   else if (string == "Most Memory") {
      return GPU_selection_method::most_memory;
   }
   else if (string == "Use CPU") {
      return GPU_selection_method::use_cpu;
   }

   return GPU_selection_method::highest_performance;
}

auto to_string(const Antialiasing_method quality) noexcept -> std::string
{
   using namespace std::literals;

   switch (quality) {
   case Antialiasing_method::none:
      return "none"s;
   case Antialiasing_method::cmaa2:
      return "CMAA2"s;
   case Antialiasing_method::msaax4:
      return "MSAAx4"s;
   case Antialiasing_method::msaax8:
      return "MSAAx8"s;
   }

   std::terminate();
}

auto to_sample_count(const Antialiasing_method quality) noexcept -> std::size_t
{
   using namespace std::literals;

   switch (quality) {
   case Antialiasing_method::none:
   case Antialiasing_method::cmaa2:
      return 1;
   case Antialiasing_method::msaax4:
      return 4;
   case Antialiasing_method::msaax8:
      return 8;
   }

   std::terminate();
}

auto aa_method_from_string(const std::string_view string) noexcept -> Antialiasing_method
{
   if (string == to_string(Antialiasing_method::none)) {
      return Antialiasing_method::none;
   }
   else if (string == to_string(Antialiasing_method::cmaa2)) {
      return Antialiasing_method::cmaa2;
   }
   else if (string == to_string(Antialiasing_method::msaax4)) {
      return Antialiasing_method::msaax4;
   }
   else if (string == to_string(Antialiasing_method::msaax8)) {
      return Antialiasing_method::msaax8;
   }
   else {
      return Antialiasing_method::none;
   }
}

auto to_string(const Anisotropic_filtering filtering) noexcept -> std::string
{
   using namespace std::literals;

   switch (filtering) {
   case Anisotropic_filtering::off:
      return "off"s;
   case Anisotropic_filtering::x2:
      return "x2"s;
   case Anisotropic_filtering::x4:
      return "x4"s;
   case Anisotropic_filtering::x8:
      return "x8"s;
   case Anisotropic_filtering::x16:
      return "x16"s;
   }

   std::terminate();
}

auto to_sample_count(const Anisotropic_filtering filtering) noexcept -> std::size_t
{
   using namespace std::literals;

   switch (filtering) {
   case Anisotropic_filtering::x2:
      return 2;
   case Anisotropic_filtering::x4:
      return 4;
   case Anisotropic_filtering::x8:
      return 8;
   case Anisotropic_filtering::x16:
      return 16;
   default:
      return 1;
   }
}

auto anisotropic_filtering_from_string(const std::string_view string) noexcept
   -> Anisotropic_filtering
{
   if (string == to_string(Anisotropic_filtering::off)) {
      return Anisotropic_filtering::off;
   }
   else if (string == to_string(Anisotropic_filtering::x2)) {
      return Anisotropic_filtering::x2;
   }
   else if (string == to_string(Anisotropic_filtering::x4)) {
      return Anisotropic_filtering::x4;
   }
   else if (string == to_string(Anisotropic_filtering::x8)) {
      return Anisotropic_filtering::x8;
   }
   else if (string == to_string(Anisotropic_filtering::x16)) {
      return Anisotropic_filtering::x16;
   }
   else {
      return Anisotropic_filtering::off;
   }
}

auto to_string(const DOF_quality quality) noexcept -> std::string
{
   using namespace std::literals;

   switch (quality) {
   case DOF_quality::ultra_performance:
      return "Ultra Performance"s;
   case DOF_quality::performance:
      return "Performance"s;
   case DOF_quality::quality:
      return "Quality"s;
   case DOF_quality::ultra_quality:
      return "Ultra Quality"s;
   }

   std::terminate();
}

auto dof_quality_from_string(const std::string_view string) noexcept -> DOF_quality
{
   if (string == to_string(DOF_quality::ultra_performance)) {
      return DOF_quality::ultra_performance;
   }
   else if (string == to_string(DOF_quality::performance)) {
      return DOF_quality::performance;
   }
   else if (string == to_string(DOF_quality::quality)) {
      return DOF_quality::quality;
   }
   else if (string == to_string(DOF_quality::ultra_quality)) {
      return DOF_quality::ultra_quality;
   }
   else {
      return DOF_quality::quality;
   }
}

auto to_string(const SSAO_quality quality) noexcept -> std::string
{
   using namespace std::literals;

   switch (quality) {
   case SSAO_quality::fastest:
      return "Fastest"s;
   case SSAO_quality::fast:
      return "Fast"s;
   case SSAO_quality::medium:
      return "Medium"s;
   case SSAO_quality::high:
      return "High"s;
   case SSAO_quality::highest:
      return "Highest"s;
   }

   std::terminate();
}

auto ssao_quality_from_string(const std::string_view string) noexcept -> SSAO_quality
{
   if (string == to_string(SSAO_quality::fastest)) {
      return SSAO_quality::fastest;
   }
   else if (string == to_string(SSAO_quality::fast)) {
      return SSAO_quality::fast;
   }
   else if (string == to_string(SSAO_quality::medium)) {
      return SSAO_quality::medium;
   }
   else if (string == to_string(SSAO_quality::high)) {
      return SSAO_quality::high;
   }
   else if (string == to_string(SSAO_quality::highest)) {
      return SSAO_quality::highest;
   }
   else {
      return SSAO_quality::medium;
   }
}

auto to_string(const Refraction_quality quality) noexcept -> std::string
{
   using namespace std::literals;

   switch (quality) {
   case Refraction_quality::low:
      return "Low"s;
   case Refraction_quality::medium:
      return "Medium"s;
   case Refraction_quality::high:
      return "High"s;
   case Refraction_quality::ultra:
      return "Ultra"s;
   }

   std::terminate();
}

auto refraction_quality_from_string(const std::string_view string) noexcept -> Refraction_quality
{
   if (string == to_string(Refraction_quality::low)) {
      return Refraction_quality::low;
   }
   else if (string == to_string(Refraction_quality::medium)) {
      return Refraction_quality::medium;
   }
   else if (string == to_string(Refraction_quality::high)) {
      return Refraction_quality::high;
   }
   else if (string == to_string(Refraction_quality::ultra)) {
      return Refraction_quality::ultra;
   }
   else {
      return Refraction_quality::medium;
   }
}

auto to_scale_factor(const Refraction_quality quality) noexcept -> int
{
   switch (quality) {
   case Refraction_quality::low:
      return 4;
   case Refraction_quality::medium:
      return 2;
   case Refraction_quality::high:
      return 2;
   case Refraction_quality::ultra:
      return 1;
   }

   return 2;
}

bool use_depth_refraction_mask(const Refraction_quality quality) noexcept
{
   switch (quality) {
   case Refraction_quality::low:
      return false;
   case Refraction_quality::medium:
      return false;
   case Refraction_quality::high:
      return true;
   case Refraction_quality::ultra:
      return true;
   }

   return false;
}

auto to_string(const Aspect_ratio_hud hud) noexcept -> std::string
{
   using namespace std::literals;

   switch (hud) {
   case Aspect_ratio_hud::stretch_4_3:
      return "Stretch 4_3"s;
   case Aspect_ratio_hud::centre_4_3:
      return "Centre 4_3"s;
   case Aspect_ratio_hud::stretch_16_9:
      return "Stretch 16_9"s;
   case Aspect_ratio_hud::centre_16_9:
      return "Centre 16_9"s;
   }

   std::terminate();
}

auto aspect_ratio_hud_from_string(const std::string_view string) noexcept -> Aspect_ratio_hud
{
   if (string == to_string(Aspect_ratio_hud::stretch_4_3)) {
      return Aspect_ratio_hud::stretch_4_3;
   }
   else if (string == to_string(Aspect_ratio_hud::centre_4_3)) {
      return Aspect_ratio_hud::centre_4_3;
   }
   else if (string == to_string(Aspect_ratio_hud::stretch_16_9)) {
      return Aspect_ratio_hud::stretch_16_9;
   }
   else if (string == to_string(Aspect_ratio_hud::centre_16_9)) {
      return Aspect_ratio_hud::centre_16_9;
   }
   else {
      return Aspect_ratio_hud::centre_4_3;
   }
}
}
