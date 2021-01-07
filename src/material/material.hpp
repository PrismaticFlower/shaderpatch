#pragma once

#include "../core/texture_database.hpp"
#include "com_ptr.hpp"
#include "game_rendertypes.hpp"
#include "patch_material_io.hpp"
#include "shader_factory.hpp"

#include <memory>
#include <vector>

#include <absl/container/inlined_vector.h>

#include <d3d11_4.h>

namespace sp::material {

enum class Constant_buffer_bind : std::uint32_t {
   none = 0b0u,
   vs = 0b1u,
   ps = 0b10u
};

constexpr bool marked_as_enum_flag(Constant_buffer_bind) noexcept
{
   return true;
}

struct Material {
   void update_resources(const core::Shader_resource_database& resource_database) noexcept;

   void bind_constant_buffers(ID3D11DeviceContext1& dc) noexcept;

   void bind_shader_resources(ID3D11DeviceContext1& dc) noexcept;

   static constexpr auto vs_resources_offset = 1;
   static constexpr auto ps_resources_offset = 7;

   static constexpr auto vs_cb_offset = 3;
   static constexpr auto ps_cb_offset = 2;

   using Resource_names = absl::InlinedVector<std::string, 16>;

   Rendertype overridden_rendertype;
   std::shared_ptr<Shader_set> shader;

   Constant_buffer_bind cb_bind;
   Com_ptr<ID3D11Buffer> constant_buffer;

   std::vector<Com_ptr<ID3D11ShaderResourceView>> vs_shader_resources;
   std::vector<Com_ptr<ID3D11ShaderResourceView>> ps_shader_resources;

   Com_ptr<ID3D11ShaderResourceView> fail_safe_game_texture;

   std::string name;
   std::string rendertype;
   std::string cb_name;
   std::vector<Material_property> properties;
   absl::flat_hash_map<std::string, std::string> resource_properties;
   Resource_names vs_shader_resources_names;
   Resource_names ps_shader_resources_names;
};

}
