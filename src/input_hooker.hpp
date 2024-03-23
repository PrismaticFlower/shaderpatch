#pragma once

#include "small_function.hpp"

#include <Windows.h>

namespace sp {

void install_dinput_hooks() noexcept;

void install_window_hooks(const HWND window) noexcept;

}
