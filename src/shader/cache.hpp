#pragma once

#include "bytecode_blob.hpp"
#include "com_ptr.hpp"
#include "shader_flags.hpp"

#include <cstddef>
#include <tuple>
#include <vector>

#include <absl/container/flat_hash_map.h>
#include <d3d11_4.h>

namespace sp::shader {

template<typename T>
struct Cache_entry {
   Com_ptr<T> shader;
   Bytecode_blob bytecode;
};

struct Cache_index {
   std::string group;
   std::string entrypoint;
   std::uint64_t static_flags;

   template<typename H>
   friend H AbslHashValue(H h, const Cache_index& index)
   {
      return H::combine(std::move(h), index.group, index.entrypoint, index.static_flags);
   }

   bool operator==(const Cache_index&) const noexcept = default;

   bool operator==(const std::tuple<const std::string_view&, const std::string_view&,
                                    const std::uint64_t>& right) const noexcept
   {
      return std::tie(group, entrypoint, static_flags) == right;
   }
};

struct Cache_index_vs {
   std::string group;
   std::string entrypoint;
   std::uint64_t static_flags;
   Vertex_shader_flags game_flags;

   template<typename H>
   friend H AbslHashValue(H h, const Cache_index_vs& index)
   {
      return H::combine(std::move(h), index.group, index.entrypoint,
                        index.static_flags, index.game_flags);
   }

   bool operator==(const Cache_index_vs&) const noexcept = default;

   bool operator==(
      const std::tuple<const std::string_view&, const std::string_view&,
                       const std::uint64_t&, const Vertex_shader_flags&>& right) const noexcept
   {
      return std::tie(group, entrypoint, static_flags, game_flags) == right;
   }
};

class Cache {
public:
   template<typename T>
   auto get_if(const std::string_view group_name, const std::string_view entrypoint_name,
               const std::uint64_t static_flags) const noexcept
      -> const Cache_entry<T>*
   {
      return get_if_impl(cache_map<T>(*this),
                         std::tie(group_name, entrypoint_name, static_flags));
   }

   auto get_vs_if(const std::string_view group_name,
                  const std::string_view entrypoint_name,
                  const std::uint64_t static_flags,
                  const Vertex_shader_flags game_flags) const noexcept
      -> const Cache_entry<ID3D11VertexShader>*
   {
      return get_if_impl(_vs_cache, std::tie(group_name, entrypoint_name,
                                             static_flags, game_flags));
   }

   template<typename T>
   void add(const std::string_view group_name, const std::string_view entrypoint_name,
            const std::uint64_t static_flags, Cache_entry<T> cache_entry) noexcept
   {
      auto& cache = cache_map<T>(*this);

      cache[Cache_index{.group = std::string{group_name},
                        .entrypoint = std::string{entrypoint_name},
                        .static_flags = static_flags}] = std::move(cache_entry);
   }

   void add_vs(const std::string_view group_name, const std::string_view entrypoint_name,
               const std::uint64_t static_flags, const Vertex_shader_flags game_flags,
               Cache_entry<ID3D11VertexShader> cache_entry) noexcept
   {
      _vs_cache[Cache_index_vs{.group = std::string{group_name},
                               .entrypoint = std::string{entrypoint_name},
                               .static_flags = static_flags,
                               .game_flags = game_flags}] = std::move(cache_entry);
   }

private:
   struct Hash_transparent {
      using is_transparent = void;

      template<typename T>
      auto operator()(const T& t) const noexcept
      {
         return absl::Hash<T>{}(t);
      }
   };

   template<typename K, typename V>
   using Basic_cache_map =
      absl::flat_hash_map<K, Cache_entry<V>, Hash_transparent, std::equal_to<>>;

   template<typename T>
   using Cache_map = Basic_cache_map<Cache_index, T>;

   using Cache_map_vs = Basic_cache_map<Cache_index_vs, ID3D11VertexShader>;

   template<typename C, typename K>
   auto get_if_impl(const C& container, K k) const noexcept
      -> std::add_pointer_t<const typename C::mapped_type>
   {
      if (auto it = container.find(k); it != container.end()) {
         return &it->second;
      }

      return nullptr;
   }

   template<typename T, typename S>
   static auto cache_map(S& self) noexcept
      -> std::conditional_t<std::is_const_v<S>, const Cache_map<T>&, Cache_map<T>&>
   {
      if constexpr (std::is_same_v<T, ID3D11ComputeShader>) {
         return self._cs_cache;
      }
      else if constexpr (std::is_same_v<T, ID3D11HullShader>) {
         return self._hs_cache;
      }
      else if constexpr (std::is_same_v<T, ID3D11DomainShader>) {
         return self._ds_cache;
      }
      else if constexpr (std::is_same_v<T, ID3D11GeometryShader>) {
         return self._gs_cache;
      }
      else if constexpr (std::is_same_v<T, ID3D11PixelShader>) {
         return self._ps_cache;
      }
   }

   Cache_map_vs _vs_cache;

   Cache_map<ID3D11ComputeShader> _cs_cache;
   Cache_map<ID3D11DomainShader> _ds_cache;
   Cache_map<ID3D11HullShader> _hs_cache;
   Cache_map<ID3D11GeometryShader> _gs_cache;
   Cache_map<ID3D11PixelShader> _ps_cache;
};

}
