
#include "material.hpp"
#include "../logger.hpp"

namespace sp::material {

namespace {

auto init_resources(const std::span<const std::string> resource_names,
                    const core::Shader_resource_database& resource_database) noexcept
   -> std::vector<Com_ptr<ID3D11ShaderResourceView>>
{
   std::vector<Com_ptr<ID3D11ShaderResourceView>> resources;

   for (const auto& name : resource_names) {
      if (name.empty()) {
         resources.emplace_back(nullptr);
         continue;
      }

      resources.emplace_back(resource_database.at_if(name));

      if (!resources.back()) {
         log(Log_level::warning, "Shader resource '{}' does not exist."sv, name);
      }
   }

   return resources;
}

}

void Material::update_resources(const core::Shader_resource_database& resource_database) noexcept
{
   vs_shader_resources = init_resources(vs_shader_resources_names, resource_database);
   ps_shader_resources = init_resources(ps_shader_resources_names, resource_database);
}

void Material::bind_constant_buffers(ID3D11DeviceContext1& dc) noexcept
{
   auto* const cb = constant_buffer.get();

   if ((cb_bind & Constant_buffer_bind::vs) == Constant_buffer_bind::vs)
      dc.VSSetConstantBuffers(vs_cb_offset, 1, &cb);

   if ((cb_bind & Constant_buffer_bind::ps) == Constant_buffer_bind::ps)
      dc.PSSetConstantBuffers(ps_cb_offset, 1, &cb);
}

void Material::bind_shader_resources(ID3D11DeviceContext1& dc) noexcept
{
   static_assert(sizeof(Com_ptr<ID3D11ShaderResourceView>) ==
                 sizeof(ID3D11ShaderResourceView*));

   if (!vs_shader_resources.empty())
      dc.VSSetShaderResources(vs_resources_offset, vs_shader_resources.size(),
                              reinterpret_cast<ID3D11ShaderResourceView**>(
                                 vs_shader_resources.data()));

   if (!ps_shader_resources.empty())
      dc.PSSetShaderResources(ps_resources_offset, ps_shader_resources.size(),
                              reinterpret_cast<ID3D11ShaderResourceView**>(
                                 ps_shader_resources.data()));
}

}
