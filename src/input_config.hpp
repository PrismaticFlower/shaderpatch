#pragma once

#include "small_function.hpp"

namespace sp {

enum class Input_mode {
   // Input is processed normally.
   normal,

   // Input is redirected to ImGui.
   imgui
};

struct Input_config {
   Input_mode mode = Input_mode::normal;

   unsigned int hotkey = 0;
   Small_function<void() noexcept> hotkey_func;
   Small_function<void() noexcept> screenshot_func;
   Small_function<void() noexcept> activate_app_func;
};

extern Input_config input_config;

}
