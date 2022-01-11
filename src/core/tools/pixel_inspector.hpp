#pragma once

#include "../../shader/database.hpp"
#include "../swapchain.hpp"

namespace sp::core::tools {

class Pixel_inspector {
public:
   Pixel_inspector(Com_ptr<ID3D11Device5> device, shader::Database& shaders);

   void show(ID3D11DeviceContext1& dc, Swapchain& swapchain, const HWND window,
             const UINT swapchain_scale) noexcept;

   /// @brief Unused by Pixel_inspector itself, this is here purely to save putting yet another bool in Shader_patch just for a dev tool.
   bool enabled = false;

private:
   const Com_ptr<ID3D11VertexShader> _vs;
   const Com_ptr<ID3D11PixelShader> _ps;
   const Com_ptr<ID3D11Texture2D> _texture;
   const Com_ptr<ID3D11ShaderResourceView> _srv;
   POINT _window_pos{};
};

}