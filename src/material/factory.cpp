
#include "factory.hpp"
#include "../logger.hpp"
#include "../material/constant_buffers.hpp"

namespace sp::material {

namespace {

auto make_constant_buffer(ID3D11Device5& device, const Material_config& material_config)
{
   return create_constant_buffer(device, material_config.cb_name,
                                 material_config.properties);
}

auto make_resources(const std::vector<std::string>& resource_names,
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
         log_fmt(Log_level::warning, "Shader resource '{}' does not exist."sv, name);
      }
   }

   return resources;
}

auto make_fail_safe_texture(const std::int32_t fail_safe_texture_index,
                            const std::vector<std::string>& resource_names,
                            const core::Shader_resource_database& resource_database) noexcept
   -> Com_ptr<ID3D11ShaderResourceView>
{
   if (fail_safe_texture_index == -1 ||
       fail_safe_texture_index >= resource_names.size()) {
      return nullptr;
   }

   const auto& name = resource_names[fail_safe_texture_index];
   auto resource = resource_database.at_if(name);

   if (!resource) {
      log_fmt(Log_level::warning, "Shader resource '{}' does not exist."sv, name);
   }

   return resource;
}

}

auto Factory::create_material(const Material_config& config) noexcept -> Material
{
   return material::Material{
      .overridden_rendertype = config.overridden_rendertype,
      .shader = _shader_factory.create(config.rendertype),
      .cb_shader_stages = config.cb_shader_stages,

      .constant_buffer = make_constant_buffer(*_device, config),

      .vs_shader_resources =
         make_resources(config.vs_resources, _shader_resource_database),
      .ps_shader_resources =
         make_resources(config.ps_resources, _shader_resource_database),

      .fail_safe_game_texture =
         make_fail_safe_texture(config.fail_safe_texture_index,
                                config.ps_resources, _shader_resource_database),

      .name = config.name,
      .rendertype = config.rendertype,
      .cb_name = config.cb_name,

      .properties = config.properties,

      .vs_shader_resources_names = config.vs_resources,
      .ps_shader_resources_names = config.ps_resources};
}

}
