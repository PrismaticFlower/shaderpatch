#pragma once

#include "../effects/bloom.hpp"
#include "../effects/color_grading.hpp"

#include <filesystem>
#include <utility>

#include <gsl/gsl>

#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4127)

#include <yaml-cpp/yaml.h>
#pragma warning(pop)

namespace sp::effects {

class Control {
private:
   Com_ref<IDirect3DDevice9> _device;

public:
   Control(Com_ref<IDirect3DDevice9> device) : _device{device} {}

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
      return _active = active;
   }

   bool active() const noexcept
   {
      return _active;
   }

   void show_imgui() noexcept;

   void drop_device_resources() noexcept;

   void read_config(YAML::Node config);

   effects::Color_grading color_grading{_device};
   effects::Bloom bloom{_device};

private:
   void save_params_to_yaml_file(const std::filesystem::path& save_to) noexcept;

   void load_params_from_yaml_file(const std::filesystem::path& load_from) noexcept;

   bool _enabled = false;
   bool _active = false;
   bool _open_failure = false;
   bool _save_failure = false;
};
}
