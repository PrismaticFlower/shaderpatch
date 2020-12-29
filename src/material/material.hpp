#pragma once

#include "../core/texture_database.hpp"
#include "../material/shader_factory.hpp"
#include "com_ptr.hpp"
#include "game_rendertypes.hpp"
#include "patch_material_io.hpp"

#include <memory>
#include <vector>

#include <d3d11_4.h>

namespace sp::material {

struct Material {
   Material() = default;

   Material(Material_config material_config, material::Shader_factory& shader_factory,
            const core::Shader_resource_database& texture_database,
            ID3D11Device5& device);

   void update_resources(const core::Shader_resource_database& resource_database) noexcept;

   void bind_constant_buffers(ID3D11DeviceContext1& dc) noexcept;

   void bind_shader_resources(ID3D11DeviceContext1& dc) noexcept;

   static constexpr auto vs_resources_offset = 1;
   static constexpr auto ps_resources_offset = 7;

   static constexpr auto vs_cb_offset = 3;
   static constexpr auto gs_cb_offset = 0;
   static constexpr auto ps_cb_offset = 2;

   Rendertype overridden_rendertype;
   std::shared_ptr<material::Shader_set> shader;

   Material_cb_shader_stages cb_shader_stages;
   Com_ptr<ID3D11Buffer> constant_buffer;

   std::vector<Com_ptr<ID3D11ShaderResourceView>> vs_shader_resources;
   std::vector<Com_ptr<ID3D11ShaderResourceView>> ps_shader_resources;

   Com_ptr<ID3D11ShaderResourceView> fail_safe_game_texture;

   const std::string name;
   const std::string rendertype;
   const std::string cb_name;
   std::vector<Material_property> properties;
   std::vector<std::string> vs_shader_resources_names;
   std::vector<std::string> ps_shader_resources_names;
};

}
