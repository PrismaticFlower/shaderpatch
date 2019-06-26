#pragma once

#include "com_ptr.hpp"
#include "game_rendertypes.hpp"
#include "game_texture.hpp"
#include "material_shader_factory.hpp"
#include "patch_material_io.hpp"
#include "texture_database.hpp"

#include <memory>
#include <vector>

#include <d3d11_1.h>

namespace sp::core {

struct Patch_material {
   Patch_material() = default;

   Patch_material(Material_info material_info, Material_shader_factory& shader_factory,
                  const Texture_database& texture_database,
                  ID3D11Device1& device) noexcept;

   void bind_constant_buffers(ID3D11DeviceContext1& dc) noexcept;

   void bind_shader_resources(ID3D11DeviceContext1& dc) noexcept;

   static constexpr auto vs_resources_offset = 1;
   static constexpr auto hs_resources_offset = 0;
   static constexpr auto ds_resources_offset = 0;
   static constexpr auto gs_resources_offset = 0;
   static constexpr auto ps_resources_offset = 6;

   static constexpr auto vs_cb_offset = 3;
   static constexpr auto hs_cb_offset = 1;
   static constexpr auto ds_cb_offset = 1;
   static constexpr auto gs_cb_offset = 1;
   static constexpr auto ps_cb_offset = 2;

   Rendertype overridden_rendertype;
   std::shared_ptr<Material_shader> shader;

   Material_cb_shader_stages cb_shader_stages;
   Com_ptr<ID3D11Buffer> constant_buffer;

   std::vector<Com_ptr<ID3D11ShaderResourceView>> vs_shader_resources;
   std::vector<Com_ptr<ID3D11ShaderResourceView>> hs_shader_resources;
   std::vector<Com_ptr<ID3D11ShaderResourceView>> ds_shader_resources;
   std::vector<Com_ptr<ID3D11ShaderResourceView>> gs_shader_resources;
   std::vector<Com_ptr<ID3D11ShaderResourceView>> ps_shader_resources;

   Game_texture fail_safe_game_texture;

   bool tessellation;
   D3D11_PRIMITIVE_TOPOLOGY tessellation_primitive_topology;

   std::string zprepass_shader_state;

   std::string name;
};

}
