#pragma once

#include "com_ptr.hpp"
#include "shader_metadata.hpp"
#include "shader_program.hpp"

#include <array>
#include <functional>
#include <string>
#include <type_traits>
#include <unordered_map>

#include <boost/container/flat_map.hpp>
#include <boost/container/small_vector.hpp>

#include <d3d9.h>

namespace sp {

class Shader_variations {
public:
   auto operator[](const Shader_flags flags) noexcept -> Shader_program&
   {
      return _variations[index(flags)];
   }

   auto operator[](const Shader_flags flags) const noexcept -> const Shader_program&
   {
      return _variations[index(flags)];
   }

   Shader_program& set(const Shader_flags flags, Shader_program program) noexcept
   {
      return _variations[index(flags)] = std::move(program);
   }

private:
   static constexpr int index(const Shader_flags flags) noexcept
   {
      auto i = static_cast<std::underlying_type_t<Shader_flags>>(flags);

      if (i & 64u) (i &= ~64u) |= 8u;
      if (i & 128u) (i &= ~128u) |= 32u;

      return i;
   }

   using Shader =
      std::pair<const Com_ptr<IDirect3DVertexShader9>, const Com_ptr<IDirect3DPixelShader9>>;

   std::array<Shader_program, 64> _variations;
};

class Shader_group {
public:
   Shader_variations& operator[](const std::string& name) noexcept
   {
      return _shader_entrypoints.at(name);
   }

   const Shader_variations& operator[](const std::string& name) const noexcept
   {
      return _shader_entrypoints.at(name);
   }

   Shader_variations& add(const std::string& name,
                          Shader_variations shader_variations) noexcept
   {
      return _shader_entrypoints[name] = shader_variations;
   }

private:
   boost::container::flat_map<std::string, Shader_variations, std::less<std::string>,
                              boost::container::small_vector<std::pair<std::string, Shader_variations>, 16>>
      _shader_entrypoints;
};

class Shader_database {
public:
   Shader_group& operator[](const std::string& rendertype) noexcept
   {
      return _shader_groups.at(rendertype);
   }

   const Shader_group& operator[](const std::string& rendertype) const noexcept
   {
      return _shader_groups.at(rendertype);
   }

   Shader_group& add(const std::string& rendertype, Shader_group shader_set) noexcept
   {
      return _shader_groups[rendertype] = shader_set;
   }

private:
   std::unordered_map<std::string, Shader_group> _shader_groups;
};
}
