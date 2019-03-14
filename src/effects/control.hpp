#pragma once

#include "../effects/postprocess.hpp"
#include "../effects/profiler.hpp"
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
};

class Control {
private:
   const Com_ptr<ID3D11Device1> _device;

public:
   Control(Com_ptr<ID3D11Device1> device,
           const core::Shader_group_collection& shader_groups) noexcept;

   bool enabled(const bool enabled) noexcept
   {
      _enabled = enabled;

      return _enabled && user_config.effects.enabled;
   }

   bool enabled() const noexcept
   {
      return _enabled && user_config.effects.enabled;
   }

   void config(const Effects_control_config& config) noexcept
   {
      _config = config;

      config_changed();
   }

   auto config() const noexcept -> Effects_control_config
   {
      return _config;
   }

   auto rt_format() const noexcept -> DXGI_FORMAT
   {
      if (_config.hdr_rendering) return DXGI_FORMAT_R16G16B16A16_FLOAT;

      switch (user_config.effects.color_quality) {
      case Color_quality::normal:
         return DXGI_FORMAT_R8G8B8A8_UNORM;
      case Color_quality::high:
         return DXGI_FORMAT_R10G10B10A2_UNORM;
      case Color_quality::ultra:
         return DXGI_FORMAT_R16G16B16A16_UNORM;
      }

      return DXGI_FORMAT_R8G8B8A8_UNORM;
   }

   void show_imgui(HWND game_window = nullptr) noexcept;

   void read_config(YAML::Node config);

   effects::Postprocess postprocess;
   effects::Profiler profiler;

private:
   auto output_params_to_yaml_string() noexcept -> std::string;

   void save_params_to_yaml_file(const std::filesystem::path& save_to) noexcept;

   void save_params_to_munged_file(const std::filesystem::path& save_to) noexcept;

   void load_params_from_yaml_file(const std::filesystem::path& load_from) noexcept;

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

      return node;
   }

   static bool decode(const Node& node, sp::effects::Effects_control_config& config)
   {
      using namespace std::literals;

      config = sp::effects::Effects_control_config{};

      config.hdr_rendering = node["HDRRendering"s].as<bool>(config.hdr_rendering);

      return true;
   }
};

}
