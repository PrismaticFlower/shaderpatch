#pragma once

#include "material.hpp"
#include "properties_view.hpp"

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

   bool has_constant_buffer() const noexcept
   {
      return _script_env["make_constant_buffer"] != nullptr;
   }

   auto make_constant_buffer(Properties_view properties_view) -> std::vector<std::byte>
   {
      return _script_env["make_constant_buffer"](properties_view);
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

private:
   sol::environment _script_env;
};

}
