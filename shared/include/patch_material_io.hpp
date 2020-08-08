#pragma once

#include "enum_flags.hpp"
#include "game_rendertypes.hpp"
#include "ucfb_reader.hpp"
#include "ucfb_writer.hpp"
#include "utility.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <d3d11_1.h>

#include <glm/glm.hpp>

namespace sp {

enum class Material_cb_shader_stages : std::uint32_t {
   none = 0b0u,
   vs = 0b10u,
   hs = 0b100u,
   ds = 0b1000u,
   gs = 0b10000u,
   ps = 0b100000u
};

constexpr bool marked_as_enum_flag(Material_cb_shader_stages) noexcept
{
   return true;
}

enum class Material_property_var_op : std::uint32_t {
   none,
   sqr,
   sqrt,
   exp,
   exp2,
   log,
   log2,
   rcp,
   sign
};

template<typename Type>
struct Material_var {
   constexpr Material_var() = default;

   constexpr Material_var(const Type value, const Type min, const Type max,
                          const Material_property_var_op op) noexcept
      : value{value}, min{min}, max{max}, op{op}
   {
   }

   Type value{};
   Type min{};
   Type max{};
   Material_property_var_op op = Material_property_var_op::none;
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
                               std::common_type_t<const Type> max,
                               const Material_property_var_op op) noexcept
      : name{std::move(name)}
   {
      this->value.emplace<Material_var<Type>>(value, min, max, op);
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

   Material_cb_shader_stages cb_shader_stages = Material_cb_shader_stages::none;
   std::string cb_name{};

   std::vector<std::string> vs_resources{};
   std::vector<std::string> hs_resources{};
   std::vector<std::string> ds_resources{};
   std::vector<std::string> gs_resources{};
   std::vector<std::string> ps_resources{};

   std::uint32_t fail_safe_texture_index{};

   bool tessellation = false;
   D3D11_PRIMITIVE_TOPOLOGY tessellation_primitive_topology =
      D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
};

void write_patch_material(ucfb::Writer& writer, const Material_config& config);

void write_patch_material(const std::filesystem::path& save_path,
                          const Material_config& config);

auto read_patch_material(ucfb::Reader_strict<"matl"_mn> reader) -> Material_config;
}
