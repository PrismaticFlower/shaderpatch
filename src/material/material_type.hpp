#pragma once

#include "properties_view.hpp"

#include <cstddef>
#include <filesystem>
#include <vector>

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

   auto make_constant_buffer(Properties_view properties_view) -> std::vector<std::byte>
   {
      return _script_env["make_constant_buffer"](properties_view);
   }

private:
   sol::environment _script_env;
};

}
