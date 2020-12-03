
#include "core/shader_patch.hpp"
#include "user_config.hpp"

#include <utility>

#include <dxgi1_6.h>

// Sort of a hacky file, exports a function in the .dll that spins up a Shader
// Patch instance and then saves it's shader cache to disk.

namespace sp {

namespace {

constexpr auto window_name = L"SP Cache Primer Window";

auto current_instance() noexcept -> HINSTANCE
{
   HINSTANCE instance = nullptr;

   GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, window_name, &instance);

   return instance;
}

using unique_windows_class =
   std::unique_ptr<const wchar_t, decltype([](const wchar_t* const cls) {
                      if (cls) UnregisterClassW(cls, current_instance());
                   })>;
using unique_window =
   std::unique_ptr<std::remove_pointer_t<HWND>, decltype([](const HWND window) {
                      if (window) DestroyWindow(window);
                   })>;

auto make_window() -> std::pair<unique_windows_class, unique_window>
{
   const WNDCLASSW window_class_desc{.lpfnWndProc =
                                        [](auto... args) {
                                           return DefWindowProcW(args...);
                                        },
                                     .hInstance = current_instance(),
                                     .lpszClassName = window_name};

   RegisterClassW(&window_class_desc);

   unique_windows_class cls{window_name};

   unique_window wnd{CreateWindowExW(0, window_name, window_name, 0, 0, 0, 1280, 720,
                                     nullptr, nullptr, current_instance(), nullptr)};

   return {std::move(cls), std::move(wnd)};
}

}

__declspec(dllexport) void initialize_shader_cache() noexcept
{
   // Never take this as an example that it's okay to set "user_config.*" stuff ___anywhere__
   // but it's constructor function and ImGUi function. I've only deemed this "safe" because this
   // function is very unique within the codebase and only intended to be called as part of the packaging process to
   // initialize the shader cache.
   user_config.graphics.gpu_selection_method = GPU_selection_method::use_cpu;

   auto [window_class, window] = make_window();

   Com_ptr<IDXGIFactory7> factory;
   CreateDXGIFactory2(0, IID_PPV_ARGS(factory.clear_and_assign()));

   Com_ptr<IDXGIAdapter4> adapter;
   factory->EnumWarpAdapter(__uuidof(IDXGIAdapter4), adapter.void_clear_and_assign());

   core::Shader_patch shader_patch{*adapter, window.get(), 800, 600};

   shader_patch.force_shader_cache_save_to_disk();
}
}
