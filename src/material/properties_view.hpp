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

   Properties_view(const Properties_view&) = delete;
   Properties_view& operator=(const Properties_view&) = delete;

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

      return apply_op(clamp(var.value, var.min, var.max), var.op);
   }

private:
   template<typename Type>
   static auto apply_op(const Type value,
                        [[maybe_unused]] const Material_property_var_op op) noexcept
   {
      if constexpr (std::is_same_v<Type, float> || std::is_same_v<Type, glm::vec2> ||
                    std::is_same_v<Type, glm::vec3> ||
                    std::is_same_v<Type, glm::vec4>) {
         switch (op) {
         case Material_property_var_op::sqr:
            return value * value;
         case Material_property_var_op::sqrt:
            return glm::sqrt(value);
         case Material_property_var_op::exp:
            return glm::exp(value);
         case Material_property_var_op::exp2:
            return glm::exp2(value);
         case Material_property_var_op::log:
            return glm::log(value);
         case Material_property_var_op::log2:
            return glm::log2(value);
         case Material_property_var_op::sign:
            return glm::sign(value);
         case Material_property_var_op::rcp:
            return 1.0f / value;
         }
      }

      return value;
   }

   const std::span<const Material_property> _properties;
};
}
