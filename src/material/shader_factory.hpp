#pragma once

#include "../shader/database.hpp"
#include "shader_set.hpp"

#include <tuple>

#include <absl/container/flat_hash_map.h>
#include <absl/container/inlined_vector.h>

namespace sp::material {

class Shader_factory {
public:
   using Flags = absl::InlinedVector<std::string, 16>;

   Shader_factory(Com_ptr<ID3D11Device5> device,
                  shader::Rendertypes_database& shaders) noexcept;

   auto create(std::string rendertype, Flags flags) noexcept
      -> std::shared_ptr<Shader_set>;

private:
   struct Hash {
      using is_transparent = void;

      template<typename T>
      auto operator()(const T& t) const noexcept
      {
         return absl::Hash<T>{}(t);
      }
   };

   const Com_ptr<ID3D11Device5> _device;
   shader::Rendertypes_database& _shaders;

   absl::flat_hash_map<std::tuple<std::string, Flags>, std::shared_ptr<Shader_set>, Hash, std::equal_to<>> _cache;
};

}
