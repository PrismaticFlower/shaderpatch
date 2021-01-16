#pragma once

#include <span>

#include "patch_material_io.hpp"

namespace sp::material {

class Properties_view {
public:
   explicit Properties_view(std::span<const Material_property> properties) noexcept
      : _properties{properties}
   {
   }

   Properties_view(const Properties_view&) = default;
   Properties_view& operator=(const Properties_view&) = default;

   Properties_view(Properties_view&&) = delete;
   Properties_view& operator=(Properties_view&&) = delete;

   ~Properties_view() = default;

   template<typename Type>
   auto get(const std::string_view name, const Type default_value) const -> Type
   {
      auto it =
         std::find_if(_properties.begin(), _properties.end(),
                      [name](const auto& prop) { return prop.name == name; });

      if (it == _properties.end()) {
         return default_value;
      }

      using Var_type = Material_var<Type>;

      if (!std::holds_alternative<Var_type>(it->value)) {
         throw std::runtime_error{
            "Material property has mismatched type with constant buffer!"};
      }

      const auto& var = std::get<Var_type>(it->value);

      using std::clamp;

      return clamp(var.value, var.min, var.max);
   }

private:
   const std::span<const Material_property> _properties;
};
}
