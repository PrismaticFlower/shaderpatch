
#include "user_config.hpp"
#include "imgui/imgui_ext.hpp"
#include "overloaded.hpp"
#include "shader_patch_version.hpp"
#include "string_utilities.hpp"
#include "user_config_descriptions.hpp"

#include <filesystem>
#include <fstream>

#include <fmt/format.h>

namespace sp {

namespace {

auto to_string_view(const GPU_selection_method method) noexcept -> std::string_view
{
   using namespace std::literals;

   switch (method) {
   case GPU_selection_method::highest_performance:
      return "Highest Performance"sv;
   case GPU_selection_method::lowest_power_usage:
      return "Lowest Power Usage"sv;
   case GPU_selection_method::highest_feature_level:
      return "Highest Feature Level"sv;
   case GPU_selection_method::most_memory:
      return "Most Memory"sv;
   case GPU_selection_method::use_cpu:
      return "Use CPU"sv;
   }

   std::terminate();
}

auto gpu_selection_method_from_string_view(const std::string_view string) noexcept
   -> GPU_selection_method
{
   if (string == to_string_view(GPU_selection_method::highest_performance)) {
      return GPU_selection_method::highest_performance;
   }
   else if (string == to_string_view(GPU_selection_method::lowest_power_usage)) {
      return GPU_selection_method::lowest_power_usage;
   }
   else if (string == to_string_view(GPU_selection_method::highest_feature_level)) {
      return GPU_selection_method::highest_feature_level;
   }
   else if (string == to_string_view(GPU_selection_method::most_memory)) {
      return GPU_selection_method::most_memory;
   }
   else if (string == to_string_view(GPU_selection_method::use_cpu)) {
      return GPU_selection_method::use_cpu;
   }

   return GPU_selection_method::highest_performance;
}

auto to_string_view(const Antialiasing_method quality) noexcept -> std::string_view
{
   using namespace std::literals;

   switch (quality) {
   case Antialiasing_method::none:
      return "none"sv;
   case Antialiasing_method::cmaa2:
      return "CMAA2"sv;
   case Antialiasing_method::msaax4:
      return "MSAAx4"sv;
   case Antialiasing_method::msaax8:
      return "MSAAx8"sv;
   }

   std::terminate();
}

auto aa_method_from_string_view(const std::string_view string) noexcept -> Antialiasing_method
{
   if (string == to_string_view(Antialiasing_method::none)) {
      return Antialiasing_method::none;
   }
   else if (string == to_string_view(Antialiasing_method::cmaa2)) {
      return Antialiasing_method::cmaa2;
   }
   else if (string == to_string_view(Antialiasing_method::msaax4)) {
      return Antialiasing_method::msaax4;
   }
   else if (string == to_string_view(Antialiasing_method::msaax8)) {
      return Antialiasing_method::msaax8;
   }
   else {
      return Antialiasing_method::none;
   }
}

auto to_string_view(const Anisotropic_filtering filtering) noexcept -> std::string_view
{
   using namespace std::literals;

   switch (filtering) {
   case Anisotropic_filtering::off:
      return "off"sv;
   case Anisotropic_filtering::x2:
      return "x2"sv;
   case Anisotropic_filtering::x4:
      return "x4"sv;
   case Anisotropic_filtering::x8:
      return "x8"sv;
   case Anisotropic_filtering::x16:
      return "x16"sv;
   }

   std::terminate();
}

auto anisotropic_filtering_from_string_view(const std::string_view string) noexcept
   -> Anisotropic_filtering
{
   if (string == to_string_view(Anisotropic_filtering::off)) {
      return Anisotropic_filtering::off;
   }
   else if (string == to_string_view(Anisotropic_filtering::x2)) {
      return Anisotropic_filtering::x2;
   }
   else if (string == to_string_view(Anisotropic_filtering::x4)) {
      return Anisotropic_filtering::x4;
   }
   else if (string == to_string_view(Anisotropic_filtering::x8)) {
      return Anisotropic_filtering::x8;
   }
   else if (string == to_string_view(Anisotropic_filtering::x16)) {
      return Anisotropic_filtering::x16;
   }
   else {
      return Anisotropic_filtering::off;
   }
}

auto to_string_view(const DOF_quality quality) noexcept -> std::string_view
{
   using namespace std::literals;

   switch (quality) {
   case DOF_quality::ultra_performance:
      return "Ultra Performance"sv;
   case DOF_quality::performance:
      return "Performance"sv;
   case DOF_quality::quality:
      return "Quality"sv;
   case DOF_quality::ultra_quality:
      return "Ultra Quality"sv;
   }

   std::terminate();
}

auto dof_quality_from_string_view(const std::string_view string) noexcept -> DOF_quality
{
   if (string == to_string_view(DOF_quality::ultra_performance)) {
      return DOF_quality::ultra_performance;
   }
   else if (string == to_string_view(DOF_quality::performance)) {
      return DOF_quality::performance;
   }
   else if (string == to_string_view(DOF_quality::quality)) {
      return DOF_quality::quality;
   }
   else if (string == to_string_view(DOF_quality::ultra_quality)) {
      return DOF_quality::ultra_quality;
   }
   else {
      return DOF_quality::quality;
   }
}

auto to_string_view(const SSAO_quality quality) noexcept -> std::string_view
{
   using namespace std::literals;

   switch (quality) {
   case SSAO_quality::fastest:
      return "Fastest"sv;
   case SSAO_quality::fast:
      return "Fast"sv;
   case SSAO_quality::medium:
      return "Medium"sv;
   case SSAO_quality::high:
      return "High"sv;
   case SSAO_quality::highest:
      return "Highest"sv;
   }

   std::terminate();
}

auto ssao_quality_from_string_view(const std::string_view string) noexcept -> SSAO_quality
{
   if (string == to_string_view(SSAO_quality::fastest)) {
      return SSAO_quality::fastest;
   }
   else if (string == to_string_view(SSAO_quality::fast)) {
      return SSAO_quality::fast;
   }
   else if (string == to_string_view(SSAO_quality::medium)) {
      return SSAO_quality::medium;
   }
   else if (string == to_string_view(SSAO_quality::high)) {
      return SSAO_quality::high;
   }
   else if (string == to_string_view(SSAO_quality::highest)) {
      return SSAO_quality::highest;
   }
   else {
      return SSAO_quality::medium;
   }
}

auto to_string_view(const Refraction_quality quality) noexcept -> std::string_view
{
   using namespace std::literals;

   switch (quality) {
   case Refraction_quality::low:
      return "Low"sv;
   case Refraction_quality::medium:
      return "Medium"sv;
   case Refraction_quality::high:
      return "High"sv;
   case Refraction_quality::ultra:
      return "Ultra"sv;
   }

   std::terminate();
}

auto refraction_quality_from_string_view(const std::string_view string) noexcept
   -> Refraction_quality
{
   if (string == to_string_view(Refraction_quality::low)) {
      return Refraction_quality::low;
   }
   else if (string == to_string_view(Refraction_quality::medium)) {
      return Refraction_quality::medium;
   }
   else if (string == to_string_view(Refraction_quality::high)) {
      return Refraction_quality::high;
   }
   else if (string == to_string_view(Refraction_quality::ultra)) {
      return Refraction_quality::ultra;
   }
   else {
      return Refraction_quality::medium;
   }
}

auto to_string_view(const Aspect_ratio_hud hud) noexcept -> std::string_view
{
   using namespace std::literals;

   switch (hud) {
   case Aspect_ratio_hud::stretch_4_3:
      return "svtretch 4_3"sv;
   case Aspect_ratio_hud::centre_4_3:
      return "Centre 4_3"sv;
   case Aspect_ratio_hud::stretch_16_9:
      return "svtretch 16_9"sv;
   case Aspect_ratio_hud::centre_16_9:
      return "Centre 16_9"sv;
   }

   std::terminate();
}

auto aspect_ratio_hud_from_string_view(const std::string_view string) noexcept -> Aspect_ratio_hud
{
   if (string == to_string_view(Aspect_ratio_hud::stretch_4_3)) {
      return Aspect_ratio_hud::stretch_4_3;
   }
   else if (string == to_string_view(Aspect_ratio_hud::centre_4_3)) {
      return Aspect_ratio_hud::centre_4_3;
   }
   else if (string == to_string_view(Aspect_ratio_hud::stretch_16_9)) {
      return Aspect_ratio_hud::stretch_16_9;
   }
   else if (string == to_string_view(Aspect_ratio_hud::centre_16_9)) {
      return Aspect_ratio_hud::centre_16_9;
   }
   else {
      return Aspect_ratio_hud::centre_4_3;
   }
}

}

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

User_config::~User_config()
{
   if (config_changed) save_file("shader patch.yml", "~TEMP shader patch.yml");
}

void User_config::show_imgui() noexcept
{
   bool changed = false;

   ImGui::Begin("User Config", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

   if (ImGui::CollapsingHeader("User Interface", ImGuiTreeNodeFlags_DefaultOpen)) {
      const auto color_picker = [](const char* name, std::array<std::uint8_t, 3>& color) {
         std::array<float, 3> rgb_color{color[0] / 255.f, color[1] / 255.f,
                                        color[2] / 255.f};

         const bool changed = ImGui::ColorEdit3(name, rgb_color.data());

         color = {static_cast<std::uint8_t>(rgb_color[0] * 255),
                  static_cast<std::uint8_t>(rgb_color[1] * 255),
                  static_cast<std::uint8_t>(rgb_color[2] * 255)};

         return changed;
      };

      changed |= color_picker("Friend Color", ui.friend_color);
      changed |= color_picker("Foe Color", ui.foe_color);
   }

   if (ImGui::CollapsingHeader("Display", ImGuiTreeNodeFlags_DefaultOpen)) {
      changed |= ImGui::Checkbox("V-Sync", &display.v_sync);
   }

   if (ImGui::CollapsingHeader("Graphics", ImGuiTreeNodeFlags_DefaultOpen)) {
      if (ImGui::BeginCombo("Anti-Aliasing Method",
                            to_string_view(graphics.antialiasing_method).data())) {
         for (const Antialiasing_method method :
              {Antialiasing_method::none, Antialiasing_method::cmaa2,
               Antialiasing_method::msaax4, Antialiasing_method::msaax8}) {
            if (ImGui::Selectable(to_string_view(method).data(),
                                  method == graphics.antialiasing_method)) {
               graphics.antialiasing_method = method;
               changed = true;
            }
         }

         ImGui::EndCombo();
      }

      changed |= ImGui::Checkbox("Supersample Alpha Test",
                                 &graphics.supersample_alpha_test);

      if (ImGui::BeginCombo("Anisotropic Filtering",
                            to_string_view(graphics.anisotropic_filtering).data())) {
         for (const Anisotropic_filtering filtering :
              {Anisotropic_filtering::off, Anisotropic_filtering::x2,
               Anisotropic_filtering::x4, Anisotropic_filtering::x8,
               Anisotropic_filtering::x16}) {
            if (ImGui::Selectable(to_string_view(filtering).data(),
                                  filtering == graphics.anisotropic_filtering)) {
               graphics.anisotropic_filtering = filtering;
               changed = true;
            }
         }

         ImGui::EndCombo();
      }

      if (ImGui::BeginCombo("Refraction Quality",
                            to_string_view(graphics.refraction_quality).data())) {
         for (const Refraction_quality quality :
              {Refraction_quality::low, Refraction_quality::medium,
               Refraction_quality::high, Refraction_quality::ultra}) {
            if (ImGui::Selectable(to_string_view(quality).data(),
                                  quality == graphics.refraction_quality)) {
               graphics.refraction_quality = quality;
               changed = true;
            }
         }

         ImGui::EndCombo();
      }

      changed |= ImGui::Checkbox("Enable Order-Independent Transparency",
                                 &graphics.enable_oit);

      changed |= ImGui::Checkbox("Enable Alternative Post Processing",
                                 &graphics.enable_alternative_postprocessing);

      changed |= ImGui::Checkbox("Allow Vertex Soft Skinning",
                                 &graphics.allow_vertex_soft_skinning);

      changed |= ImGui::Checkbox("Enable Scene Blur", &graphics.enable_scene_blur);

      ImGui::Checkbox("Enable 16-Bit Color Channel Rendering",
                      &graphics.enable_16bit_color_rendering);

      changed |= ImGui::Checkbox("Disable Light Brightness Rescaling",
                                 &graphics.disable_light_brightness_rescaling);

      changed |= ImGui::Checkbox("Enable User Effects Config",
                                 &graphics.enable_user_effects_config);
      changed |= ImGui::InputText("User Effects Config", graphics.user_effects_config);
      changed |= ImGui::Checkbox("Enable Auto User Effects Config",
                                 &graphics.enable_user_effects_auto_config);
   }

   if (ImGui::CollapsingHeader("Effects", ImGuiTreeNodeFlags_DefaultOpen)) {
      changed |= ImGui::Checkbox("Bloom", &effects.bloom);
      changed |= ImGui::Checkbox("Vignette", &effects.vignette);
      changed |= ImGui::Checkbox("Film Grain", &effects.film_grain);
      changed |=
         ImGui::Checkbox("Allow Colored Film Grain", &effects.colored_film_grain);
      changed |= ImGui::Checkbox("SSAO", &effects.ssao);

      if (ImGui::BeginCombo("SSAO Quality",
                            to_string_view(effects.ssao_quality).data())) {
         for (const SSAO_quality quality : {
                 SSAO_quality::fastest,
                 SSAO_quality::fast,
                 SSAO_quality::medium,
                 SSAO_quality::high,
                 SSAO_quality::highest,
              }) {
            if (ImGui::Selectable(to_string_view(quality).data(),
                                  quality == effects.ssao_quality)) {
               effects.ssao_quality = quality;
               changed = true;
            }
         }

         ImGui::EndCombo();
      }

      changed |= ImGui::Checkbox("Depth of Field", &effects.ssao);

      if (ImGui::BeginCombo("Depth of Field Quality",
                            to_string_view(effects.dof_quality).data())) {
         for (const DOF_quality quality :
              {DOF_quality::ultra_performance, DOF_quality::performance,
               DOF_quality::quality, DOF_quality::ultra_quality}) {
            if (ImGui::Selectable(to_string_view(quality).data(),
                                  quality == effects.dof_quality)) {
               effects.dof_quality = quality;
               changed = true;
            }
         }

         ImGui::EndCombo();
      }
   }

   ImGui::Text("Shader Patch v%s", current_shader_patch_version_string.c_str());

   ImGui::End();

   config_changed |= changed;
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

   display.aspect_ratio_hack_hud = aspect_ratio_hud_from_string_view(
      config["Display"s]["Aspect Ratio Hack HUD Handling"s].as<std::string_view>(
         to_string_view(display.aspect_ratio_hack_hud)));

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

   graphics.gpu_selection_method = gpu_selection_method_from_string_view(
      config["Graphics"s]["GPU Selection Method"s].as<std::string>("Highest Performance"s));

   graphics.antialiasing_method = aa_method_from_string_view(
      config["Graphics"s]["Anti-Aliasing Method"s].as<std::string_view>(
         to_string_view(graphics.antialiasing_method)));

   graphics.supersample_alpha_test =
      config["Graphics"s]["Supersample Alpha Test"s].as<bool>();

   graphics.anisotropic_filtering = anisotropic_filtering_from_string_view(
      config["Graphics"s]["Anisotropic Filtering"s].as<std::string_view>(
         to_string_view(graphics.anisotropic_filtering)));

   graphics.refraction_quality = refraction_quality_from_string_view(
      config["Graphics"s]["Refraction Quality"s].as<std::string_view>(
         to_string_view(graphics.refraction_quality)));

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

   effects.ssao_quality = ssao_quality_from_string_view(
      config["Effects"s]["SSAO Quality"s].as<std::string_view>(
         to_string_view(effects.ssao_quality)));

   effects.dof = config["Effects"s]["Depth of Field"s].as<bool>(effects.dof);

   effects.dof_quality = dof_quality_from_string_view(
      config["Effects"s]["Depth of Field Quality"s].as<std::string_view>(
         to_string_view(effects.dof_quality)));

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

   std::ofstream out{path};

   out << config;
}

void User_config::save_file(const std::string& path, const std::string& temp_path)
{
   try {
      std::ofstream out{temp_path};

      constexpr static auto printify = overloaded{
         [](const auto& v) -> std::string_view { return to_string_view(v); },
         [](const std::string& v) -> std::string_view { return v; },
         [](const std::string_view v) -> std::string_view { return v; },
         [](const bool& v) -> std::string_view { return v ? "yes"sv : "no"sv; }};
      constexpr static auto printify_dynamic =
         overloaded{[](const std::uint32_t v) -> std::string {
                       return fmt::format("{}", v);
                    },
                    [](const std::filesystem::path& v) -> std::string {
                       return v.string();
                    },
                    [](const std::array<std::uint32_t, 2>& v) -> std::string {
                       return fmt::format("[{}, {}]", v[0], v[1]);
                    },
                    [](const std::array<std::uint8_t, 3>& v) -> std::string {
                       return fmt::format("[{:d}, {:d}, {:d}]", v[0], v[1], v[2]);
                    }};
      constexpr static auto line_break = "\n\n"sv;
      constexpr static auto indention = "   "sv;
      constexpr static auto max_comment_line_size = 100;

      const auto write_value = [&](const std::string_view name,
                                   const std::string_view value) {
         auto description = user_config_descriptions.at(name);

         for (Line line : Lines_iterator{description}) {
            out << indention << "#"sv;

            std::size_t current_line_length = 0;

            for (auto& token : Token_iterator{line.string}) {
               current_line_length += (token.size() + 1); // + 1 for ' '

               if (current_line_length >= max_comment_line_size) {
                  out << '\n' << indention << "#"sv;
                  current_line_length = 0;
               }

               out << ' ' << token;
            }

            out << '\n';
         }

         out << indention << name << ": "sv << value << line_break;
      };

      out << "Shader Patch Enabled: yes"sv << line_break;

      out << "Display: "sv << line_break;

      write_value("V-Sync", printify(display.v_sync));
      write_value("Treat 800x600 As Interface",
                  printify(display.treat_800x600_as_interface));
      write_value("Stretch Interface", printify(display.stretch_interface));
      write_value("Display Scaling Aware", printify(display.dpi_aware));
      write_value("Display Scaling", printify(display.dpi_scaling));
      write_value("DSR-VSR Display Scaling", printify(display.dsr_vsr_scaling));
      write_value("Scalable Fonts", printify(display.scalable_fonts));
      write_value("Enable Game Perceived Resolution Override",
                  printify(display.enable_game_perceived_resolution_override));
      write_value("Game Perceived Resolution Override",
                  printify_dynamic(std::array<std::uint32_t, 2>{
                     display.game_perceived_resolution_override_width,
                     display.game_perceived_resolution_override_height}));
      write_value("Aspect Ratio Hack", printify(display.aspect_ratio_hack));
      write_value("Aspect Ratio Hack HUD Handling",
                  printify(display.aspect_ratio_hack_hud));
      write_value("Override Resolution", printify(display.override_resolution));
      write_value("Override Resolution Screen Percent",
                  printify_dynamic(display.override_resolution_screen_percent));

      out << "User Interface: "sv << line_break;

      write_value("Extra UI Scaling", printify_dynamic(ui.extra_ui_scaling));
      write_value("Friend Color", printify_dynamic(ui.friend_color));
      write_value("Foe Color", printify_dynamic(ui.foe_color));

      out << "Graphics: "sv << line_break;

      write_value("GPU Selection Method", printify(graphics.gpu_selection_method));
      write_value("Anti-Aliasing Method", printify(graphics.antialiasing_method));
      write_value("Supersample Alpha Test", printify(graphics.supersample_alpha_test));
      write_value("Anisotropic Filtering", printify(graphics.anisotropic_filtering));
      write_value("Refraction Quality", printify(graphics.refraction_quality));
      write_value("Enable Order-Independent Transparency",
                  printify(graphics.enable_oit));
      write_value("Enable Alternative Post Processing",
                  printify(graphics.enable_alternative_postprocessing));
      write_value("Allow Vertex Soft Skinning",
                  printify(graphics.allow_vertex_soft_skinning));
      write_value("Enable Scene Blur", printify(graphics.enable_scene_blur));
      write_value("Enable 16-Bit Color Channel Rendering",
                  printify(graphics.enable_16bit_color_rendering));
      write_value("Disable Light Brightness Rescaling",
                  printify(graphics.disable_light_brightness_rescaling));
      write_value("Enable User Effects Config",
                  printify(graphics.enable_user_effects_config));
      write_value("User Effects Config", printify(graphics.user_effects_config));
      write_value("Enable Auto User Effects Config",
                  printify(graphics.enable_user_effects_auto_config));
      write_value("Use Direct3D 11 on 12", printify(graphics.use_d3d11on12));

      out << "Effects: "sv << line_break;

      write_value("Bloom", printify(effects.bloom));
      write_value("Vignette", printify(effects.vignette));
      write_value("Film Grain", printify(effects.film_grain));
      write_value("Allow Colored Film Grain", printify(effects.colored_film_grain));
      write_value("SSAO", printify(effects.ssao));
      write_value("SSAO Quality", printify(effects.ssao_quality));
      write_value("Depth of Field", printify(effects.dof));
      write_value("Depth of Field Quality", printify(effects.dof_quality));

      out << "Developer: "sv << line_break;

      write_value("Screen Toggle", printify_dynamic(developer.toggle_key));
      write_value("Monitor BFront2.log", printify(developer.monitor_bfront2_log));
      write_value("Allow Event Queries", printify(developer.allow_event_queries));
      write_value("Use D3D11 Debug Layer", printify(developer.use_d3d11_debug_layer));
      write_value("Use DXGI 1.2 Factory", printify(developer.use_dxgi_1_2_factory));
      write_value("Shader Cache Path", printify_dynamic(developer.shader_cache_path));
      write_value("Shader Definitions Path",
                  printify_dynamic(developer.shader_definitions_path));
      write_value("Shader Source Path", printify_dynamic(developer.shader_source_path));
      write_value("Material Scripts Path",
                  printify_dynamic(developer.material_scripts_path));
      write_value("Scalable Font Name", printify_dynamic(developer.scalable_font_name));

      out.close();

      std::filesystem::rename(temp_path, path);
   }
   catch (std::exception&) {
   }
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

}
