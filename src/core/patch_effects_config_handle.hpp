#pragma once

#include <functional>

#include "../effects/control.hpp"
#include "small_function.hpp"

namespace sp::core {

class Patch_effects_config_handle {
public:
   Patch_effects_config_handle(Small_function<void() noexcept> on_destruction) noexcept
      : _on_destruction{std::move(on_destruction)}
   {
   }

   ~Patch_effects_config_handle()
   {
      _on_destruction();
   }

   Patch_effects_config_handle(const Patch_effects_config_handle&) = default;
   Patch_effects_config_handle& operator=(const Patch_effects_config_handle&) = default;

   Patch_effects_config_handle(Patch_effects_config_handle&&) = default;
   Patch_effects_config_handle& operator=(Patch_effects_config_handle&&) = default;

private:
   Small_function<void() noexcept> _on_destruction;
};

}
