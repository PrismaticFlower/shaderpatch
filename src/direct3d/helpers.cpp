
#include "helpers.hpp"
#include "../core/shader_patch.hpp"
#include "buffer.hpp"

namespace sp::d3d9 {

auto create_triangle_fan_quad_ibuf(core::Shader_patch& shader_patch) noexcept
   -> Com_ptr<IDirect3DIndexBuffer9>
{
   constexpr std::array<std::uint16_t, 6> contents{0, 1, 2, 0, 2, 3};

   auto buffer = Index_buffer::create(shader_patch, sizeof(contents), false);

   void* buffer_data_ptr;
   buffer->Lock(0, sizeof(contents), &buffer_data_ptr, D3DLOCK_NOSYSLOCK);

   std::memcpy(buffer_data_ptr, &contents, sizeof(contents));

   buffer->Unlock();

   return buffer;
}

}
