#pragma once

#include "com_ptr.hpp"
#include "depthstencil.hpp"
#include "shader_database.hpp"

#include <d3d11_4.h>

namespace sp::core {

class Depth_msaa_resolver {
public:
   Depth_msaa_resolver(ID3D11Device5& device,
                       const Shader_database& shader_database) noexcept;

   ~Depth_msaa_resolver() = default;
   Depth_msaa_resolver(const Depth_msaa_resolver&) = default;
   Depth_msaa_resolver& operator=(const Depth_msaa_resolver&) = default;
   Depth_msaa_resolver(Depth_msaa_resolver&&) = default;
   Depth_msaa_resolver& operator=(Depth_msaa_resolver&&) = default;

   void resolve(ID3D11DeviceContext4& dc, const Depthstencil& source,
                const Depthstencil& dest) const noexcept;

private:
   auto get_pixel_shader(const UINT sample_count) const noexcept -> ID3D11PixelShader*;

   const Com_ptr<ID3D11VertexShader> _vs;
   const Com_ptr<ID3D11PixelShader> _ps_x2;
   const Com_ptr<ID3D11PixelShader> _ps_x4;
   const Com_ptr<ID3D11PixelShader> _ps_x8;
   const Com_ptr<ID3D11DepthStencilState> _ds_state;
};

}
