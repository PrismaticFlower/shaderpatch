#pragma once

#include "../shader_database.hpp"
#include "com_ptr.hpp"
#include "com_ref.hpp"

#include <glm/glm.hpp>

#include <d3d9.h>

namespace sp::effects {

class Damage_overlay {
public:
   Damage_overlay(Com_ref<IDirect3DDevice9> device);

   void apply(const Shader_rendertype& rendertype, glm::vec4 colour,
              IDirect3DTexture9& from, IDirect3DSurface9& over) noexcept;

   void drop_device_resources() noexcept;

private:
   void setup_vetrex_stream() noexcept;

   void set_constants(glm::vec4 colour, IDirect3DTexture9& from) noexcept;

   void create_resources() noexcept;

   struct Constants {
      alignas(16) glm::vec4 color{1.0f, 1.0f, 1.0f, 0.0f};
      float step_size{1.0f};
      float sample_count{1.0f};
      std::array<float, 2> _pad{};
   };

   static_assert(sizeof(Constants) == 32);

   Com_ref<IDirect3DDevice9> _device;
   Com_ptr<IDirect3DVertexDeclaration9> _vertex_decl;
   Com_ptr<IDirect3DVertexBuffer9> _vertex_buffer;
};

}
