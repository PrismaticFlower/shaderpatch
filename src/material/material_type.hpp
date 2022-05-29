#pragma once

#include "material.hpp"
#include "properties_view.hpp"
#include "resource_info_view.hpp"
#include "shader_factory.hpp"

#include <cstddef>
#include <filesystem>
#include <vector>

#include <absl/container/flat_hash_map.h>

#pragma warning(disable : 4702)
#include <sol/sol.hpp>
#pragma warning(default : 4702)

namespace sp::material {

class Material_type {
public:
   Material_type(sol::state& lua, const std::filesystem::path& script_path)
   {
      _script_env = sol::environment{lua, sol::create, lua.globals()};

      lua.script_file(script_path.string(), _script_env);
   }

   auto constant_buffer_bind() const noexcept -> Constant_buffer_bind
   {
      return _script_env["constant_buffer_bind"];
   }

   auto fail_safe_texture_index() const noexcept -> std::uint32_t
   {
      return _script_env["fail_safe_texture_index"];
   }

   bool has_constant_buffer() const noexcept
   {
      return _script_env["make_constant_buffer"] != nullptr;
   }

   auto make_constant_buffer(Properties_view properties_view,
                             Resource_info_views resource_info_views)
      -> std::vector<std::byte>
   {
      return _script_env["make_constant_buffer"](properties_view, resource_info_views);
   }

   bool has_resources() const noexcept
   {
      return _script_env["fill_resource_vec"] != nullptr;
   }

   auto make_resources_vec(Properties_view properties_view,
                           const absl::flat_hash_map<std::string, std::string>& resource_props)
      -> Material::Resource_names
   {
      Material::Resource_names resources;

      _script_env["fill_resource_vec"](properties_view, resource_props, resources);

      return resources;
   }

   bool has_vs_resources() const noexcept
   {
      return _script_env["fill_vs_resource_vec"] != nullptr;
   }

   auto make_vs_resources_vec(Properties_view properties_view,
                              const absl::flat_hash_map<std::string, std::string>& resource_props)
      -> Material::Resource_names
   {
      Material::Resource_names resources;

      _script_env["fill_vs_resource_vec"](properties_view, resource_props, resources);

      return resources;
   }

   bool has_shader_flags() const noexcept
   {
      return _script_env["get_shader_flags"] != nullptr;
   }

   auto get_shader_flags(Properties_view properties_view) -> Shader_factory::Flags
   {
      Shader_factory::Flags flags;

      _script_env["get_shader_flags"](properties_view, flags);

      return flags;
   }

private:
   sol::environment _script_env;
};

}
