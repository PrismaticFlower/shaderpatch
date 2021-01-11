#pragma once

#include "enum_flags.hpp"
#include "game_rendertypes.hpp"
#include "ucfb_reader.hpp"
#include "ucfb_writer.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <absl/container/flat_hash_map.h>
#include <glm/glm.hpp>

namespace sp {

template<typename Type>
struct Material_var {
   constexpr Material_var() = default;

   constexpr Material_var(const Type value, const Type min, const Type max) noexcept
      : value{value}, min{min}, max{max}
   {
   }

   Type value{};
   Type min{};
   Type max{};
   std::uint32_t abi_padding = 0;
};

struct Material_property {
   using Value = std::variant<
      Material_var<float>, Material_var<glm::vec2>, Material_var<glm::vec3>,
      Material_var<glm::vec4>, // float values
      Material_var<std::int32_t>, Material_var<glm::ivec2>, Material_var<glm::ivec3>,
      Material_var<glm::ivec4>, // int values
      Material_var<std::uint32_t>, Material_var<glm::uvec2>, Material_var<glm::uvec3>,
      Material_var<glm::uvec4>, // uint values
      Material_var<bool>        // bool values
      >;

   template<typename Type>
   constexpr Material_property(std::string name, const Type value,
                               std::common_type_t<const Type> min,
                               std::common_type_t<const Type> max) noexcept
      : name{std::move(name)}
   {
      this->value.emplace<Material_var<Type>>(value, min, max);
   }

   template<typename Type>
   constexpr Material_property(std::string name, const Type value) noexcept
      : name{std::move(name)}
   {
      this->value.emplace<Material_var<Type>>(value,
                                              Type{std::numeric_limits<Type>::lowest()},
                                              Type{std::numeric_limits<Type>::max()});
   }

   Material_property(std::string name, const Value value) noexcept
      : name{std::move(name)}, value{value}
   {
   }

   std::string name;
   Value value; // I mean, it's not pretty but it's type-safe and does exactly what we need.
};

struct Material_config {
   std::string name;
   std::string rendertype;
   Rendertype overridden_rendertype;

   std::vector<Material_property> properties;

   std::string cb_name = "none";

   absl::flat_hash_map<std::string, std::string> resources{};
};

void write_patch_material(ucfb::File_writer& writer, const Material_config& config);

void write_patch_material(const std::filesystem::path& save_path,
                          const Material_config& config);

auto read_patch_material(ucfb::Reader_strict<"matl"_mn> reader) -> Material_config;
}
