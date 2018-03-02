#include "../logger.hpp"

#include <string_view>

#include <Windows.h>
#include <d3d9.h>

namespace sp::direct3d {

using namespace std::string_literals;

namespace {
constexpr auto failed_message =
   "One or more required Direct3D 9 features are unavailable. See 'shader "
   "patch.log' for more details."
   "\n\nPress Cancel to exit."
   "\nPress Try Again to break into a debugger."
   "\nPress Continue to ignore the missing features and attempt to continue.";

bool feature_test(const bool result, std::string_view error_msg)
{
   if (!result) log(Log_level::error, error_msg);

   return result;
}

bool supports_texture_format(IDirect3D9& d3d, D3DFORMAT format)
{
   return (SUCCEEDED(d3d.CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                                           0, D3DRTYPE_TEXTURE, format)) == 1);
}
}

void check_required_features(IDirect3D9& d3d) noexcept
{
   bool supported = true;

   D3DCAPS9 caps;

   d3d.GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);

   supported &= feature_test(caps.VertexShaderVersion >= 0xfffe0300u,
                             "Device does not support Shader Model 3.0."sv);
   supported &= feature_test(caps.PixelShaderVersion >= 0xfffe0300u,
                             "Device does not support Shader Model 3.0."sv);

   supported &= feature_test(supports_texture_format(d3d, D3DFMT_DXT5),
                             "Device does not support BC3 (DXT(4|5) textures."sv);

   if (!supported) {
      const auto action = MessageBoxA(nullptr, failed_message, "Device Unsupported",
                                      MB_CANCELTRYCONTINUE | MB_ICONERROR |
                                         MB_SETFOREGROUND | MB_TOPMOST);

      if (action == IDCANCEL)
         TerminateProcess(GetCurrentProcess(), 1);
      else if (action == IDTRYAGAIN)
         DebugBreak();
   }
}
}
