#pragma once

#include "com_ptr.hpp"
#include "shader_database.hpp"

#include <glm/glm.hpp>

#include <d3d11_1.h>

namespace sp::core {

class Image_stretcher {
public:
   Image_stretcher(ID3D11Device1& device, const Shader_database& shader_database) noexcept;

   ~Image_stretcher() = default;
   Image_stretcher(const Image_stretcher&) = default;
   Image_stretcher& operator=(const Image_stretcher&) = default;
   Image_stretcher(Image_stretcher&&) = default;
   Image_stretcher& operator=(Image_stretcher&&) = default;

   void stretch(ID3D11DeviceContext1& dc, const D3D11_BOX& source_box,
                ID3D11ShaderResourceView& source, ID3D11UnorderedAccessView& dest,
                const D3D11_BOX& dest_box) const noexcept;

private:
   struct alignas(16) Input_vars {
      glm::uvec2 dest_start;
      glm::uvec2 dest_end;
      glm::uvec2 dest_length;

      glm::uvec2 src_start;
      glm::uvec2 src_length;

      glm::uvec2 _padding;
   };

   static_assert(sizeof(Input_vars) == 48);

   const Com_ptr<ID3D11ComputeShader> _x_8_y_8_shader;
   const Com_ptr<ID3D11Buffer> _constant_buffer;
};

}
