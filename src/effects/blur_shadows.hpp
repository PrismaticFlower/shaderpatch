#pragma once

#include "../shader_database.hpp"
#include "com_ptr.hpp"
#include "com_ref.hpp"
#include "rendertarget_allocator.hpp"

namespace sp::effects {

#include <d3d9.h>

class Blur_shadows {
public:
   Blur_shadows(Com_ref<IDirect3DDevice9> device);

   void apply(const Shader_group& shaders, Rendertarget_allocator& allocator,
              IDirect3DTexture9& depth, IDirect3DTexture9& from_to) noexcept;

   void drop_device_resources() noexcept;

private:
   void do_pass(IDirect3DSurface9& dest, IDirect3DTexture9& source) const noexcept;

   void set_rt_metrics_state(glm::ivec2 resolution) const noexcept;

   void set_direction_state(glm::ivec2 resolution, glm::ivec2 direction) const noexcept;

   void setup_samplers() const noexcept;

   void setup_vetrex_stream() noexcept;

   void create_resources() noexcept;

   Com_ref<IDirect3DDevice9> _device;
   Com_ptr<IDirect3DVertexDeclaration9> _vertex_decl;
   Com_ptr<IDirect3DVertexBuffer9> _vertex_buffer;
};

}
