
#include "factory.hpp"
#include "../logger.hpp"
#include "../user_config.hpp"
#include "material_type.hpp"
#include "sol_create_usertypes.hpp"

#include "../core/d3d11_helpers.hpp"

#pragma warning(disable : 4702)
#include <sol/sol.hpp>
#pragma warning(default : 4702)

using namespace std::literals;

namespace sp::material {

namespace {

auto make_constant_buffer(ID3D11Device5& device, Material_type& material_type,
                          Properties_view properties_view) -> Com_ptr<ID3D11Buffer>
{
   const auto buffer = material_type.make_constant_buffer(properties_view);

   return core::create_immutable_constant_buffer(device, std::span{buffer});
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

struct Factory_lua_state {
   sol::state lua;
};

Factory::Factory(Com_ptr<ID3D11Device5> device,
                 shader::Rendertypes_database& shader_rendertypes_database,
                 core::Shader_resource_database& shader_resource_database)
   : _device{device},
     _shader_factory{device, shader_rendertypes_database},
     _shader_resource_database{shader_resource_database},
     _lua_state_owner{std::make_unique<Factory_lua_state>()}
{
   auto& lua = _lua_state_owner->lua;

   lua.open_libraries(sol::lib::base, sol::lib::string, sol::lib::math, sol::lib::table);

   sol_create_usertypes(lua);

   // TODO: Make this async.
   for (auto script_entry : std::filesystem::directory_iterator{
           user_config.developer.material_scripts_path}) {
      if (!script_entry.is_regular_file()) continue;

      auto script_path = script_entry.path();

      if (script_path.extension() != L".lua"sv) continue;

      _material_types.emplace(script_path.stem().string(),
                              std::make_unique<Material_type>(lua, script_path));
   }
}

Factory::~Factory() = default;

auto Factory::create_material(const Material_config& config) noexcept -> Material
{
   material::Material material{.overridden_rendertype = config.overridden_rendertype,
                               .shader = _shader_factory.create(config.rendertype),
                               .cb_shader_stages = config.cb_shader_stages,

                               .fail_safe_game_texture =
                                  make_fail_safe_texture(config.fail_safe_texture_index,
                                                         config.ps_resources,
                                                         _shader_resource_database),

                               .name = config.name,
                               .rendertype = config.rendertype,
                               .cb_name = config.cb_name,

                               .properties = config.properties,

                               .vs_shader_resources_names = config.vs_resources,
                               .ps_shader_resources_names = config.ps_resources};

   update_material(material);

   return material;
}

void Factory::update_material(material::Material& material) noexcept
{
   if (auto it = _material_types.find(material.cb_name); it != _material_types.end()) {
      auto& material_type = it->second;

      material.constant_buffer =
         make_constant_buffer(*_device, *material_type,
                              Properties_view{material.properties});
   }

   material.vs_shader_resources =
      make_resources(material.vs_shader_resources_names, _shader_resource_database);
   material.ps_shader_resources =
      make_resources(material.ps_shader_resources_names, _shader_resource_database);
}

}
