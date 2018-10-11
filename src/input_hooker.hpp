#pragma once

#include <Windows.h>

#include <functional>

namespace sp {

enum class Input_mode {
   // Input is processed normally.
   normal,

   // Input is redirected to ImGui.
   imgui
};

void initialize_input_hooks(const DWORD thread_id) noexcept;

void set_input_window(const DWORD thread_id, const HWND window) noexcept;

void set_input_mode(const Input_mode mode) noexcept;

void set_input_hotkey(const WPARAM hotkey) noexcept;

void set_input_hotkey_func(std::function<void()> function) noexcept;
}
