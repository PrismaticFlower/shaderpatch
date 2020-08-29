#pragma once

#include "../effects/cmaa2.hpp"
#include "../effects/ffx_cas.hpp"
#include "../effects/postprocess.hpp"
#include "../effects/profiler.hpp"
#include "../effects/ssao.hpp"
#include "com_ptr.hpp"

#include <filesystem>
#include <utility>

#include <Windows.h>

#include <gsl/gsl>

#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4127)

#include <yaml-cpp/yaml.h>
#pragma warning(pop)

namespace sp::effects {

struct Effects_control_config {
   bool hdr_rendering = false;
   bool oit_requested = false;
   bool disable_light_brightness_rescaling = false;
   bool fp_rendertargets = false;
};

class Control {
public:
   Control(Com_ptr<ID3D11Device4> device,
           const core::Shader_group_collection& shader_groups) noexcept;

   bool enabled(const bool enabled) noexcept;

   bool enabled() const noexcept;

   void config(const Effects_control_config& config) noexcept
   {
      _config = config;

      config_changed();
   }

   auto config() const noexcept -> Effects_control_config
   {
      return _config;
   }

   void show_imgui(HWND game_window = nullptr) noexcept;

   void read_config(YAML::Node config);

   effects::Postprocess postprocess;
   effects::CMAA2 cmaa2;
   effects::SSAO ssao;
   effects::FFX_cas ffx_cas;
   effects::Profiler profiler;

private:
   auto output_params_to_yaml_string() noexcept -> std::string;

   void save_params_to_yaml_file(const std::filesystem::path& save_to) noexcept;

   void save_params_to_munged_file(const std::filesystem::path& save_to) noexcept;

   void load_params_from_yaml_file(const std::filesystem::path& load_from) noexcept;

   void imgui_save_widget(const HWND game_window) noexcept;

   void show_post_processing_imgui() noexcept;

   void config_changed() noexcept;

   bool _enabled = false;
   bool _open_failure = false;
   bool _save_failure = false;

   Effects_control_config _config{};
};
}

namespace YAML {
template<>
struct convert<sp::effects::Effects_control_config> {
   static Node encode(const sp::effects::Effects_control_config& config)
   {
      using namespace std::literals;

      YAML::Node node;

      node["HDRRendering"s] = config.hdr_rendering;
      node["RequestOIT"s] = config.oit_requested;
      node["DisableLightBrightnessRescaling"s] =
         config.disable_light_brightness_rescaling;
      node["FPRenderTargets"s] = config.fp_rendertargets;

      return node;
   }

   static bool decode(const Node& node, sp::effects::Effects_control_config& config)
   {
      using namespace std::literals;

      config = sp::effects::Effects_control_config{};

      config.hdr_rendering = node["HDRRendering"s].as<bool>(config.hdr_rendering);
      config.oit_requested = node["RequestOIT"s].as<bool>(config.oit_requested);
      config.disable_light_brightness_rescaling =
         node["DisableLightBrightnessRescaling"s].as<bool>(
            config.disable_light_brightness_rescaling);
      config.fp_rendertargets =
         node["FPRenderTargets"s].as<bool>(config.fp_rendertargets);

      return true;
   }
};
}
