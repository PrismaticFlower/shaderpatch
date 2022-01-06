#pragma once

#include "../shader/database.hpp"
#include "com_ptr.hpp"
#include "profiler.hpp"

#include <d3d11_4.h>

namespace sp::effects {

struct Mask_nan_input {
   ID3D11RenderTargetView& rtv;
   UINT width = 0;
   UINT height = 0;
};

class Mask_nan {
public:
   Mask_nan(Com_ptr<ID3D11Device5> device, shader::Database& shaders) noexcept;

   void apply(ID3D11DeviceContext4& dc, const Mask_nan_input input,
              Profiler& profiler) noexcept;

private:
   const Com_ptr<ID3D11VertexShader> _vs;
   const Com_ptr<ID3D11PixelShader> _ps;
   const Com_ptr<ID3D11BlendState> _blend_state;
};

}