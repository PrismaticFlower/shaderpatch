#pragma once

#include "../effects/postprocess.hpp"
#include "../effects/scene_blur.hpp"

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
   Com_ref<IDirect3DDevice9> _device;

public:
   Control(Com_ref<IDirect3DDevice9> device, Post_aa_quality aa_quality)
      : _device{device}, postprocess{device, aa_quality}
   {
   }

   bool enabled(bool enable) noexcept
   {
      return _enabled = enable;
   }

   bool enabled() const noexcept
   {
      return _enabled;
   }

   bool active(bool active) noexcept
   {
      postprocess.hdr_state(_config.hdr_rendering ? Hdr_state::hdr : Hdr_state::stock);

      return _active = active;
   }

   bool active() const noexcept
   {
      return _active;
   }

   void config(const Effects_control_config& config) noexcept
   {
      _config = config;
   }

   auto config() const noexcept -> Effects_control_config
   {
      return _config;
   }

   void show_imgui(HWND game_window = nullptr) noexcept;

   void drop_device_resources() noexcept;

   void read_config(YAML::Node config);

   effects::Postprocess postprocess;
   effects::Scene_blur scene_blur{_device};

private:
   auto output_params_to_yaml_string() noexcept -> std::string;

   void save_params_to_yaml_file(const std::filesystem::path& save_to) noexcept;

   void save_params_to_munged_file(const std::filesystem::path& save_to) noexcept;

   void load_params_from_yaml_file(const std::filesystem::path& load_from) noexcept;

   bool _enabled = false;
   bool _active = false;
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
