#pragma once

#include "../shader_database.hpp"
#include "com_ptr.hpp"
#include "com_ref.hpp"
#include "rendertarget_allocator.hpp"

#include <d3d9.h>

namespace sp::effects {

class Scene_blur {
public:
   Scene_blur(Com_ref<IDirect3DDevice9> device);

   void apply(const Shader_database& shaders, Rendertarget_allocator& allocator,
              IDirect3DTexture9& from_to) noexcept;

   void drop_device_resources() noexcept;

private:
   void setup_state(D3DFORMAT format) noexcept;

   void setup_vetrex_stream() noexcept;

   void create_resources() noexcept;

   void do_pass(IDirect3DSurface9& dest, IDirect3DTexture9& source,
                glm::vec2 direction) const noexcept;

   Com_ref<IDirect3DDevice9> _device;
   Com_ptr<IDirect3DVertexDeclaration9> _vertex_decl;
   Com_ptr<IDirect3DVertexBuffer9> _vertex_buffer;
};

}
