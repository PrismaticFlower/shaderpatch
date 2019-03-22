
#include "patch_material.hpp"
#include "d3d11_helpers.hpp"

namespace sp::core {

namespace {

auto init_textures(const std::vector<std::string>& textures,
                   const Texture_database& texture_database) noexcept
   -> std::vector<Com_ptr<ID3D11ShaderResourceView>>
{
   std::vector<Com_ptr<ID3D11ShaderResourceView>> resources;

   for (const auto& texture : textures) {
      if (texture.empty()) {
         resources.emplace_back(nullptr);
         continue;
      }

      resources.emplace_back(texture_database.get(texture));
   }

   return resources;
}

auto init_fail_safe_texture(const std::vector<Com_ptr<ID3D11ShaderResourceView>>& shader_resources,
                            const std::int32_t fail_safe_texture_index) noexcept -> Game_texture
{
   Expects(fail_safe_texture_index == -1 ||
           fail_safe_texture_index < shader_resources.size());

   if (fail_safe_texture_index == -1) return {};

   return {shader_resources[fail_safe_texture_index],
           shader_resources[fail_safe_texture_index]};
}

}

Patch_material::Patch_material(Material_info material_info,
                               Material_shader_factory& shader_factory,
                               const Texture_database& texture_database,
                               ID3D11Device1& device) noexcept
   : overridden_rendertype{material_info.overridden_rendertype},
     shader{shader_factory.create(material_info.rendertype)},
     cb_shader_stages{material_info.cb_shader_stages},
     constant_buffer{
        create_immutable_constant_buffer(device, material_info.constant_buffer)},
     vs_shader_resources{init_textures(material_info.vs_textures, texture_database)},
     hs_shader_resources{init_textures(material_info.hs_textures, texture_database)},
     ds_shader_resources{init_textures(material_info.ds_textures, texture_database)},
     gs_shader_resources{init_textures(material_info.gs_textures, texture_database)},
     ps_shader_resources{init_textures(material_info.ps_textures, texture_database)},
     fail_safe_game_texture{
        init_fail_safe_texture(ps_shader_resources,
                               material_info.fail_safe_texture_index)},
     tessellation{material_info.tessellation},
     tessellation_primitive_topology{material_info.tessellation_primitive_topology},
     name{std::move(material_info.name)}
{
}

void Patch_material::bind_constant_buffers(ID3D11DeviceContext1& dc) noexcept
{
   auto* const cb = constant_buffer.get();

   if ((cb_shader_stages & Material_cb_shader_stages::vs) ==
       Material_cb_shader_stages::vs)
      dc.VSSetConstantBuffers(vs_cb_offset, 1, &cb);

   if ((cb_shader_stages & Material_cb_shader_stages::hs) ==
       Material_cb_shader_stages::hs)
      dc.HSSetConstantBuffers(hs_cb_offset, 1, &cb);

   if ((cb_shader_stages & Material_cb_shader_stages::ds) ==
       Material_cb_shader_stages::ds)
      dc.DSSetConstantBuffers(ds_cb_offset, 1, &cb);

   if ((cb_shader_stages & Material_cb_shader_stages::gs) ==
       Material_cb_shader_stages::gs)
      dc.GSSetConstantBuffers(gs_cb_offset, 1, &cb);

   if ((cb_shader_stages & Material_cb_shader_stages::ps) ==
       Material_cb_shader_stages::ps)
      dc.PSSetConstantBuffers(ps_cb_offset, 1, &cb);
}

void Patch_material::bind_shader_resources(ID3D11DeviceContext1& dc) noexcept
{
   static_assert(sizeof(Com_ptr<ID3D11ShaderResourceView>) ==
                 sizeof(ID3D11ShaderResourceView*));

   if (!vs_shader_resources.empty())
      dc.VSSetShaderResources(vs_resources_offset, vs_shader_resources.size(),
                              reinterpret_cast<ID3D11ShaderResourceView**>(
                                 vs_shader_resources.data()));

   if (!hs_shader_resources.empty())
      dc.HSSetShaderResources(hs_resources_offset, hs_shader_resources.size(),
                              reinterpret_cast<ID3D11ShaderResourceView**>(
                                 hs_shader_resources.data()));

   if (!ds_shader_resources.empty())
      dc.DSSetShaderResources(ds_resources_offset, ds_shader_resources.size(),
                              reinterpret_cast<ID3D11ShaderResourceView**>(
                                 ds_shader_resources.data()));

   if (!gs_shader_resources.empty())
      dc.GSSetShaderResources(gs_resources_offset, gs_shader_resources.size(),
                              reinterpret_cast<ID3D11ShaderResourceView**>(
                                 gs_shader_resources.data()));

   if (!ps_shader_resources.empty())
      dc.PSSetShaderResources(ps_resources_offset, ps_shader_resources.size(),
                              reinterpret_cast<ID3D11ShaderResourceView**>(
                                 ps_shader_resources.data()));
}

}
