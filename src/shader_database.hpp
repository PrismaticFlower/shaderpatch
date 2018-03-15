#pragma once

#include "com_ptr.hpp"
#include "shader_metadata.hpp"

#include <array>
#include <string>
#include <type_traits>
#include <unordered_map>

#include <d3d9.h>

namespace sp {

class Shader_group;
class Shader_variations;

class Shader_database {
public:
   Shader_group& operator[](const std::string_view rendertype) noexcept
   {
      return _shader_groups.at(rendertype);
   }

   const Shader_group& operator[](const std::string_view rendertype) const noexcept
   {
      return _shader_groups.at(rendertype);
   }

   Shader_group& add(const std::string_view rendertype, Shader_group shader_set) noexcept
   {
      _shader_groups.emplace(std::make_pair(rendertype, shader_set));
   }

private:
   std::unordered_map<std::string_view, Shader_group> _shader_groups;
};

class Shader_group {
public:
   using Shader =
      std::pair<const Com_ptr<IDirect3DVertexShader9>, const Com_ptr<IDirect3DPixelShader9>>;

   Shader_variations& operator[](const std::string_view entrypoint) noexcept
   {
      return _shader_entrypoints.at(entrypoint);
   }

   const Shader_variations& operator[](const std::string_view entrypoint) const noexcept
   {
      return _shader_entrypoints.at(entrypoint);
   }

   Shader_variations& add(const std::string_view entrypoint,
                          Shader_variations shader_variations) noexcept
   {
      _shader_entrypoints.emplace(std::make_pair(entrypoint, shader_variations));
   }

private:
   std::unordered_map<std::string_view, Shader_variations> _shader_entrypoints;
};

class Shader_variations {
public:
   using Shader =
      std::pair<const Com_ptr<IDirect3DVertexShader9>, const Com_ptr<IDirect3DPixelShader9>>;

   const Shader& operator[](const Shader_flags flags) const noexcept
   {
      return _variations[index(flags)];
   }

   Shader& add(const Shader_flags flags, Shader shader) noexcept
   {
      _variations[index(flags)] = std::move(shader);
   }

private:
   static constexpr int index(const Shader_flags flags) noexcept
   {
      auto i = static_cast<std::underlying_type_t<Shader_flags>>(flags);

      if (i >= 64) i << 1;

      return i;
   }

   std::array<Shader, 36> _variations;
};
}
