#pragma once

#include "small_function.hpp"

#include <Windows.h>

namespace sp {

void install_window_hooks(const HWND window) noexcept;

}
