#pragma once

#include "com_ptr.hpp"
#include "logger.hpp"
#include "shader_metadata.hpp"
#include "shader_program.hpp"

#include <array>
#include <functional>
#include <iomanip>
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

   void bind(IDirect3DDevice9& device,
             const Shader_flags flags = Shader_flags::none) const noexcept
   {
      (*this)[flags].bind(device);
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
   Shader_variations& at(const std::string& shader_state) noexcept
   {
      if (auto shader = find(shader_state); shader) {
         return *shader;
      }
      else {
         log_and_terminate("Unable to find shader variation set for shader state "sv,
                           std::quoted(shader_state), '.');
      }
   }

   const Shader_variations& at(const std::string& shader_state) const noexcept
   {
      if (auto shader = find(shader_state); shader) {
         return *shader;
      }
      else {
         log_and_terminate("Unable to find shader variation set for shader state "sv,
                           std::quoted(shader_state), '.');
      }
   }

   Shader_variations* find(const std::string& shader_state) noexcept
   {
      if (auto result = _shader_entrypoints.find(shader_state);
          result != std::end(_shader_entrypoints)) {
         return &result->second;
      }

      return nullptr;
   }

   const Shader_variations* find(const std::string& shader_state) const noexcept
   {
      if (auto result = _shader_entrypoints.find(shader_state);
          result != std::cend(_shader_entrypoints)) {
         return &result->second;
      }

      return nullptr;
   }

   Shader_variations& add(const std::string& shader_state,
                          Shader_variations shader_variations) noexcept
   {
      if (_shader_entrypoints.count(shader_state)) {
         log_and_terminate("Attempt to add shader variation set for already defined shader state "sv,
                           std::quoted(shader_state), '.');
      }

      return _shader_entrypoints[shader_state] = shader_variations;
   }

private:
   boost::container::flat_map<std::string, Shader_variations, std::less<std::string>,
                              boost::container::small_vector<std::pair<std::string, Shader_variations>, 16>>
      _shader_entrypoints;
};

class Shader_database {
public:
   Shader_group& at(const std::string& rendertype) noexcept
   {
      if (auto group = _shader_groups.find(rendertype);
          group != std::end(_shader_groups)) {
         return group->second;
      }
      else {
         log_and_terminate("Unable to find shader group for rendertype "sv,
                           std::quoted(rendertype), '.');
      }
   }

   const Shader_group& at(const std::string& rendertype) const noexcept
   {
      if (auto group = _shader_groups.find(rendertype);
          group != std::cend(_shader_groups)) {
         return group->second;
      }
      else {
         log_and_terminate("Unable to find shader group for rendertype "sv,
                           std::quoted(rendertype), '.');
      }
   }

   Shader_group& add(const std::string& rendertype, Shader_group shader_group) noexcept
   {
      return _shader_groups[rendertype] = shader_group;
   }

private:
   std::unordered_map<std::string, Shader_group> _shader_groups;
};
}
