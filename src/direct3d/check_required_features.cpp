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

constexpr auto ati1_format = static_cast<D3DFORMAT>(MAKEFOURCC('A', 'T', 'I', '1'));
constexpr auto ati2_format = static_cast<D3DFORMAT>(MAKEFOURCC('A', 'T', 'I', '2'));

bool supports_texture_format(IDirect3D9& d3d, D3DFORMAT format)
{
   return (SUCCEEDED(d3d.CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                                           0, D3DRTYPE_TEXTURE, format)) == 1);
}

bool supports_texture_3d_format(IDirect3D9& d3d, D3DFORMAT format)
{
   return (SUCCEEDED(d3d.CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                                           0, D3DRTYPE_VOLUMETEXTURE, format)) == 1);
}

bool supports_render_texture_format(IDirect3D9& d3d, D3DFORMAT format)
{
   return (SUCCEEDED(d3d.CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                                           D3DFMT_X8R8G8B8, D3DUSAGE_RENDERTARGET,
                                           D3DRTYPE_TEXTURE, format)) == 1);
}

}

void check_required_features(IDirect3D9& d3d) noexcept
{
   bool supported = true;

   const auto feature_test = [&supported](bool test_result,
                                          std::string_view error_message) {
      if (!test_result) log(Log_level::error, error_message);

      supported &= test_result;
   };

   D3DCAPS9 caps;

   d3d.GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);

   feature_test(caps.VertexShaderVersion >= 0xfffe0300u,
                "Device does not support Shader Model 3.0."sv);
   feature_test(caps.PixelShaderVersion >= 0xfffe0300u,
                "Device does not support Shader Model 3.0."sv);

   feature_test(supports_texture_format(d3d, D3DFMT_DXT1),
                "Device does not support BC1 (DXT1) textures."sv);
   feature_test(supports_texture_format(d3d, D3DFMT_DXT5),
                "Device does not support BC3 (DXT(4|5) textures."sv);
   feature_test(supports_texture_format(d3d, ati1_format),
                "Device does not support ATI1 (BC4) textures."sv);
   feature_test(supports_texture_format(d3d, ati2_format),
                "Device does not support ATI2 textures."sv);
   feature_test(supports_texture_format(d3d, D3DFMT_L16),
                "Device does not support L16 (R16) textures."sv);

   feature_test(supports_texture_3d_format(d3d, D3DFMT_A16B16G16R16F),
                "Device does not support 3D A16B16G16R16F (R16G16B16A16_FLOAT) textures."sv);

   feature_test(supports_render_texture_format(d3d, D3DFMT_R32F),
                "Device does not support rendering to R32F textures."sv);
   feature_test(supports_render_texture_format(d3d, D3DFMT_G16R16F),
                "Device does not support rendering to G16R16F (R16G16F) textures."sv);

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
