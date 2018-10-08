#pragma once

#include "com_ptr.hpp"
#include "logger.hpp"
#include "shader_flags.hpp"
#include "shader_metadata.hpp"
#include "shader_program.hpp"

#include <array>
#include <functional>
#include <iomanip>
#include <string>
#include <type_traits>
#include <unordered_map>

#include <d3d9.h>

namespace sp {

namespace detail {

enum class Shader_type { vertex, pixel };

template<Shader_type type>
struct Shader_type_info {
};

template<>
struct Shader_type_info<Shader_type::vertex> {
   using Resource_type = IDirect3DVertexShader9;
   using Flags_type = Vertex_shader_flags;

   static void bind(IDirect3DDevice9& device, Resource_type* shader) noexcept
   {
      device.SetVertexShader(shader);
   }
};

template<>
struct Shader_type_info<Shader_type::pixel> {
   using Resource_type = IDirect3DPixelShader9;
   using Flags_type = Pixel_shader_flags;

   static void bind(IDirect3DDevice9& device, Resource_type* shader) noexcept
   {
      device.SetPixelShader(shader);
   }
};

}

template<detail::Shader_type type>
class Shader_entrypoint {
public:
   Shader_entrypoint() = default;
   explicit Shader_entrypoint(const Shader_entrypoint&) = default;
   Shader_entrypoint(Shader_entrypoint&&) = default;
   Shader_entrypoint& operator=(Shader_entrypoint&&) = default;

   Shader_entrypoint& operator=(const Shader_entrypoint&) = delete;

   using Resource = typename detail::Shader_type_info<type>::Resource_type;
   using Game_flags = typename detail::Shader_type_info<type>::Flags_type;

   Resource* get(const std::uint16_t static_flags = 0,
                 const Game_flags game_flags = {}) const noexcept
   {
      if (const auto it = _variations.find(index(static_flags, game_flags));
          it != _variations.end()) {
         return it->second.get();
      }
      else {
         return nullptr;
      }
   }

   void bind(IDirect3DDevice9& device, const std::uint16_t static_flags = 0,
             const Game_flags game_flags = {}) const noexcept
   {
      detail::Shader_type_info<type>::bind(device, get(static_flags, game_flags));
   }

   void insert(Com_ptr<Resource> shader, const std::uint16_t static_flags,
               const Game_flags game_flags) noexcept
   {
      _variations[index(static_flags, game_flags)] = std::move(shader);
   }

private:
   static constexpr std::uint32_t index(const std::uint16_t static_flags,
                                        const Game_flags game_flags) noexcept
   {
      static_assert(sizeof(Game_flags) <= sizeof(std::uint32_t));

      return static_cast<std::uint32_t>(game_flags) | (static_flags << 16u);
   }

   std::unordered_map<std::uint32_t, Com_ptr<Resource>> _variations;
};

using Vertex_shader_entrypoint = Shader_entrypoint<detail::Shader_type::vertex>;
using Pixel_shader_entrypoint = Shader_entrypoint<detail::Shader_type::pixel>;

template<detail::Shader_type type>
class Shader_entrypoints {
public:
   Shader_entrypoints() = default;
   explicit Shader_entrypoints(const Shader_entrypoints&) = default;
   Shader_entrypoints(Shader_entrypoints&&) = default;
   Shader_entrypoints& operator=(Shader_entrypoints&&) = default;

   Shader_entrypoints& operator=(const Shader_entrypoints&) = delete;

   using Entrypoint = Shader_entrypoint<type>;

   Entrypoint& at(const std::string& entrypoint) noexcept
   {
      if (auto shader = find(_shader_entrypoints); shader) {
         return *shader;
      }
      else {
         log_and_terminate("Unable to find shader shader entrypoint "sv,
                           std::quoted(entrypoint), '.');
      }
   }

   const Entrypoint& at(const std::string& entrypoint) const noexcept
   {
      if (auto shader = find(entrypoint); shader) {
         return *shader;
      }
      else {
         log_and_terminate("Unable to find shader entrypoint "sv,
                           std::quoted(entrypoint), '.');
      }
   }

   Entrypoint* find(const std::string& entrypoint) noexcept
   {
      if (auto result = _shader_entrypoints.find(entrypoint);
          result != std::end(_shader_entrypoints)) {
         return &result->second;
      }

      return nullptr;
   }

   const Entrypoint* find(const std::string& entrypoint) const noexcept
   {
      if (auto result = _shader_entrypoints.find(entrypoint);
          result != std::cend(_shader_entrypoints)) {
         return &result->second;
      }

      return nullptr;
   }

   Entrypoint& insert(const std::string& entrypoint_name, Entrypoint entrypoint) noexcept
   {
      if (_shader_entrypoints.count(entrypoint_name)) {
         log_and_terminate("Attempt to replace already loaded shader entrypoint "sv,
                           std::quoted(entrypoint_name), '.');
      }

      return _shader_entrypoints[entrypoint_name] = std::move(entrypoint);
   }

private:
   std::unordered_map<std::string, Entrypoint> _shader_entrypoints;
};

struct Shader_group {
   Shader_group() = default;
   explicit Shader_group(const Shader_group&) = default;
   Shader_group(Shader_group&&) = default;
   Shader_group& operator=(Shader_group&&) = default;

   Shader_group& operator=(const Shader_group&) = delete;

   Shader_entrypoints<detail::Shader_type::vertex> vertex;
   Shader_entrypoints<detail::Shader_type::pixel> pixel;
};

class Shader_group_collection {
public:
   Shader_group_collection() = default;
   Shader_group_collection(Shader_group_collection&&) = default;
   Shader_group_collection& operator=(Shader_group_collection&&) = default;

   Shader_group_collection(const Shader_group_collection&) = delete;
   Shader_group_collection& operator=(const Shader_group_collection&) = delete;

   Shader_group& at(const std::string& group) noexcept
   {
      if (auto shader = find(group); shader) {
         return *shader;
      }
      else {
         log_and_terminate("Unable to find shader group "sv, std::quoted(group), '.');
      }
   }

   const Shader_group& at(const std::string& group) const noexcept
   {
      if (auto shader = find(group); shader) {
         return *shader;
      }
      else {
         log_and_terminate("Unable to find shader group "sv, std::quoted(group), '.');
      }
   }

   Shader_group* find(const std::string& group) noexcept
   {
      if (auto result = _groups.find(group); result != _groups.end()) {
         return &result->second;
      }

      return nullptr;
   }

   const Shader_group* find(const std::string& group) const noexcept
   {
      if (auto result = _groups.find(group); result != _groups.cend()) {
         return &result->second;
      }

      return nullptr;
   }

   Shader_group& insert(const std::string& group_name, Shader_group group) noexcept
   {
      if (auto [it, success] = _groups.try_emplace(group_name, std::move(group));
          success) {
         return it->second;
      }
      else {
         log_and_terminate("Attempt to replace already existing shader group "sv,
                           std::quoted(group_name), '.');
      }
   }

private:
   std::unordered_map<std::string, Shader_group> _groups;
};

class Shader_state {
public:
   Shader_state(Vertex_shader_entrypoint vertex_shader_entrypoint,
                std::uint16_t vertex_static_flags,
                Pixel_shader_entrypoint pixel_shader_entrypoint,
                std::uint16_t pixel_static_flags) noexcept
      : _vertex_shader_entrypoint{std::move(vertex_shader_entrypoint)},
        _vertex_static_flags{vertex_static_flags},
        _pixel_shader_entrypoint{std::move(pixel_shader_entrypoint)},
        _pixel_static_flags{pixel_static_flags} {};

   explicit Shader_state(const Shader_state&) = default;
   Shader_state(Shader_state&&) = default;
   Shader_state& operator=(Shader_state&&) = default;

   Shader_state& operator=(const Shader_state&) = delete;

   // This function is NOT threadsafe. //
   void bind(IDirect3DDevice9& device, const Vertex_shader_flags vertex_flags = {},
             const Pixel_shader_flags pixel_flags = {}) const noexcept
   {
      if (auto result = _variation_cache.find(index(vertex_flags, pixel_flags));
          result != _variation_cache.cend()) {
         device.SetVertexShader(result->second.first.get());
         device.SetPixelShader(result->second.second.get());
      }
      else {
         bind_and_add_to_cache(device, vertex_flags, pixel_flags);
      }
   }

private:
   static constexpr std::uint32_t index(const Vertex_shader_flags vertex_flags,
                                        const Pixel_shader_flags pixel_flags) noexcept
   {
      static_assert(sizeof(Vertex_shader_flags) <= sizeof(std::uint32_t));
      static_assert(sizeof(Pixel_shader_flags) <= sizeof(std::uint32_t));

      return static_cast<std::uint32_t>(vertex_flags) |
             (static_cast<std::uint32_t>(pixel_flags) << 16u);
   }

   void bind_and_add_to_cache(IDirect3DDevice9& device,
                              const Vertex_shader_flags vertex_flags,
                              const Pixel_shader_flags pixel_flags) const noexcept
   {
      IDirect3DVertexShader9* vertex = nullptr;
      IDirect3DPixelShader9* pixel = nullptr;

      if (vertex = _vertex_shader_entrypoint.get(_vertex_static_flags, vertex_flags);
          vertex) {
         device.SetVertexShader(vertex);
      }
      else {
         log_and_terminate("Attempt to bind nonexistent shader variation."sv);
      }

      if (pixel = _pixel_shader_entrypoint.get(_pixel_static_flags, pixel_flags); pixel) {
         device.SetPixelShader(pixel);
      }
      else {
         log_and_terminate("Attempt to bind nonexistent shader variation."sv);
      }

      vertex->AddRef();
      pixel->AddRef();

      _variation_cache.emplace(index(vertex_flags, pixel_flags),
                               std::pair{Com_ptr{vertex}, Com_ptr{pixel}});
   }

   mutable std::unordered_map<std::uint32_t, std::pair<Com_ptr<IDirect3DVertexShader9>, Com_ptr<IDirect3DPixelShader9>>> _variation_cache;

   const Vertex_shader_entrypoint _vertex_shader_entrypoint;
   const std::uint16_t _vertex_static_flags;
   const Pixel_shader_entrypoint _pixel_shader_entrypoint;
   const std::uint16_t _pixel_static_flags;
};

class Shader_rendertype {
public:
   Shader_rendertype() = default;
   explicit Shader_rendertype(const Shader_rendertype&) = default;
   Shader_rendertype(Shader_rendertype&&) = default;
   Shader_rendertype& operator=(Shader_rendertype&&) = default;

   Shader_rendertype& operator=(const Shader_rendertype&) = delete;

   const Shader_state& at(const std::string& state) const noexcept
   {
      if (auto result = find(state); result) {
         return *result;
      }
      else {
         log_and_terminate("Unable to find shader state "sv, std::quoted(state), '.');
      }
   }

   const Shader_state* find(const std::string& state) const noexcept
   {
      if (auto result = _states.find(state); result != _states.cend()) {
         return &result->second;
      }

      return nullptr;
   }

   auto insert(const std::string& state_name, Shader_state state) noexcept
      -> Shader_state&
   {
      if (auto [it, success] = _states.try_emplace(state_name, std::move(state));
          success) {
         return it->second;
      }
      else {
         log_and_terminate("Attempt to replace already loaded shader state "sv,
                           std::quoted(state_name), '.');
      }
   }

private:
   std::unordered_map<std::string, Shader_state> _states;
};

class Shader_rendertype_collection {
public:
   Shader_rendertype_collection() = default;
   explicit Shader_rendertype_collection(const Shader_rendertype_collection&) = default;
   Shader_rendertype_collection(Shader_rendertype_collection&&) = default;
   Shader_rendertype_collection& operator=(Shader_rendertype_collection&&) = default;

   Shader_rendertype_collection& operator=(const Shader_rendertype_collection&) = delete;

   const Shader_rendertype& at(const std::string& rendertype) const noexcept
   {
      if (auto result = find(rendertype); result) {
         return *result;
      }
      else {
         log_and_terminate("Unable to find shader rendertype "sv,
                           std::quoted(rendertype), '.');
      }
   }

   const Shader_rendertype* find(const std::string& rendertype) const noexcept
   {
      if (auto group = _rendertypes.find(rendertype); group != _rendertypes.end()) {
         return &group->second;
      }

      return nullptr;
   }

   Shader_rendertype& insert(const std::string& rendertype_name,
                             Shader_rendertype rendertype) noexcept
   {
      if (auto [it, success] =
             _rendertypes.try_emplace(rendertype_name, std::move(rendertype));
          success) {
         return it->second;
      }
      else {
         log_and_terminate("Attempt to replace already loaded shader rendertype "sv,
                           std::quoted(rendertype_name), '.');
      }
   }

private:
   std::unordered_map<std::string, Shader_rendertype> _rendertypes;
};

struct Shader_database {
   Shader_database() = default;
   Shader_database(Shader_database&&) = default;
   Shader_database& operator=(Shader_database&&) = default;

   Shader_database(const Shader_database&) = delete;
   Shader_database& operator=(const Shader_database&) = delete;

   Shader_group_collection groups;
   Shader_rendertype_collection rendertypes;
};

}
