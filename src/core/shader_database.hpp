#pragma once

#include "../logger.hpp"
#include "com_ptr.hpp"
#include "shader_flags.hpp"
#include "shader_input_element.hpp"
#include "shader_metadata.hpp"
#include "utility.hpp"

#include <array>
#include <functional>
#include <iomanip>
#include <string>
#include <tuple>
#include <unordered_map>

#include <gsl/gsl>

#include <d3d11_1.h>

namespace sp::core {

class Compute_shader_entrypoint {
public:
   Compute_shader_entrypoint() = default;
   explicit Compute_shader_entrypoint(const Compute_shader_entrypoint&) = default;
   Compute_shader_entrypoint(Compute_shader_entrypoint&&) = default;
   Compute_shader_entrypoint& operator=(Compute_shader_entrypoint&&) = default;

   Compute_shader_entrypoint& operator=(const Compute_shader_entrypoint&) = delete;

   auto get(const std::uint32_t static_flags = 0) const noexcept -> ID3D11ComputeShader*
   {
      if (const auto it = _variations.find(static_flags); it != _variations.end()) {
         return it->second.get();
      }
      else {
         return nullptr;
      }
   }

   auto copy(const std::uint32_t static_flags = 0) const noexcept
      -> Com_ptr<ID3D11ComputeShader>
   {
      if (const auto it = _variations.find(static_flags); it != _variations.end()) {
         return it->second;
      }

      log_and_terminate("Attempt to copy nonexistant shader!");
   }

   void bind(ID3D11DeviceContext& dc, const std::uint32_t static_flags = 0) const noexcept
   {
      dc.CSSetShader(get(static_flags), nullptr, 0);
   }

   void insert(Com_ptr<ID3D11ComputeShader> shader, const std::uint32_t static_flags) noexcept
   {
      _variations[static_flags] = std::move(shader);
   }

private:
   std::unordered_map<std::uint32_t, Com_ptr<ID3D11ComputeShader>> _variations;
};

class Vertex_shader_entrypoint {
public:
   Vertex_shader_entrypoint() = default;
   explicit Vertex_shader_entrypoint(const Vertex_shader_entrypoint&) = default;
   Vertex_shader_entrypoint(Vertex_shader_entrypoint&&) = default;
   Vertex_shader_entrypoint& operator=(Vertex_shader_entrypoint&&) = default;

   Vertex_shader_entrypoint& operator=(const Vertex_shader_entrypoint&) = delete;

   auto get(const std::uint16_t static_flags = 0,
            const Vertex_shader_flags game_flags = {}) const noexcept
      -> std::optional<std::tuple<ID3D11VertexShader*, gsl::span<const std::byte>, gsl::span<const Shader_input_element>>>
   {
      if (const auto it = _variations.find(index(static_flags, game_flags));
          it != _variations.end()) {
         return std::tuple{std::get<0>(it->second).get(),
                           gsl::make_span(std::get<1>(it->second)),
                           gsl::make_span(std::get<2>(it->second))};
      }
      else {
         return std::nullopt;
      }
   }

   auto copy(const std::uint16_t static_flags = 0,
             const Vertex_shader_flags game_flags = {}) const noexcept
      -> std::tuple<Com_ptr<ID3D11VertexShader>, std::vector<std::byte>, std::vector<Shader_input_element>>
   {
      if (const auto it = _variations.find(index(static_flags, game_flags));
          it != _variations.end()) {
         return it->second;
      }

      log_and_terminate("Attempt to copy nonexistant shader!");
   }

   void bind(ID3D11DeviceContext& dc, const std::uint16_t static_flags = 0,
             const Vertex_shader_flags game_flags = {}) const noexcept
   {
      if (const auto shader = get(static_flags, game_flags)) {
         dc.VSSetShader(std::get<0>(*shader), nullptr, 0);
      }
      else {
         dc.VSSetShader(nullptr, nullptr, 0);
      }
   }

   void insert(Com_ptr<ID3D11VertexShader> shader, std::vector<std::byte> bytecode,
               std::vector<Shader_input_element> input_layout,
               const std::uint16_t static_flags,
               const Vertex_shader_flags game_flags) noexcept
   {
      _variations[index(static_flags, game_flags)] = {std::move(shader),
                                                      std::move(bytecode),
                                                      std::move(input_layout)};
   }

private:
   static constexpr std::uint32_t index(const std::uint16_t static_flags,
                                        const Vertex_shader_flags game_flags) noexcept
   {
      static_assert(sizeof(Pixel_shader_flags) <= sizeof(std::uint32_t));

      return static_cast<std::uint32_t>(game_flags) | (static_flags << 16u);
   }

   std::unordered_map<std::uint32_t,
                      std::tuple<Com_ptr<ID3D11VertexShader>, std::vector<std::byte>, std::vector<Shader_input_element>>>
      _variations;
};

class Pixel_shader_entrypoint {
public:
   Pixel_shader_entrypoint() = default;
   explicit Pixel_shader_entrypoint(const Pixel_shader_entrypoint&) = default;
   Pixel_shader_entrypoint(Pixel_shader_entrypoint&&) = default;
   Pixel_shader_entrypoint& operator=(Pixel_shader_entrypoint&&) = default;

   Pixel_shader_entrypoint& operator=(const Pixel_shader_entrypoint&) = delete;

   auto get(const std::uint16_t static_flags = 0,
            const Pixel_shader_flags game_flags = {}) const noexcept -> ID3D11PixelShader*
   {
      if (const auto it = _variations.find(index(static_flags, game_flags));
          it != _variations.end()) {
         return it->second.get();
      }
      else {
         return nullptr;
      }
   }

   auto copy(const std::uint16_t static_flags = 0,
             const Pixel_shader_flags game_flags = {}) const noexcept
      -> Com_ptr<ID3D11PixelShader>
   {
      if (const auto it = _variations.find(index(static_flags, game_flags));
          it != _variations.end()) {
         return it->second;
      }

      log_and_terminate("Attempt to copy nonexistant shader!");
   }

   void bind(ID3D11DeviceContext& dc, const std::uint16_t static_flags = 0,
             const Pixel_shader_flags game_flags = {}) const noexcept
   {
      dc.PSSetShader(get(static_flags, game_flags), nullptr, 0);
   }

   void insert(Com_ptr<ID3D11PixelShader> shader, const std::uint16_t static_flags,
               const Pixel_shader_flags game_flags) noexcept
   {
      _variations[index(static_flags, game_flags)] = std::move(shader);
   }

private:
   static constexpr std::uint32_t index(const std::uint16_t static_flags,
                                        const Pixel_shader_flags game_flags) noexcept
   {
      static_assert(sizeof(Pixel_shader_flags) <= sizeof(std::uint32_t));

      return static_cast<std::uint32_t>(game_flags) | (static_flags << 16u);
   }

   std::unordered_map<std::uint32_t, Com_ptr<ID3D11PixelShader>> _variations;
};

template<typename Entrypoint>
class Shader_entrypoints {
public:
   Shader_entrypoints() = default;
   explicit Shader_entrypoints(const Shader_entrypoints&) = default;
   Shader_entrypoints(Shader_entrypoints&&) = default;
   Shader_entrypoints& operator=(Shader_entrypoints&&) = default;

   Shader_entrypoints& operator=(const Shader_entrypoints&) = delete;

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

   Shader_entrypoints<Compute_shader_entrypoint> compute;
   Shader_entrypoints<Vertex_shader_entrypoint> vertex;
   Shader_entrypoints<Pixel_shader_entrypoint> pixel;
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

   auto at(const Vertex_shader_flags vertex_flags = {}) const noexcept
      -> std::tuple<Com_ptr<ID3D11VertexShader>, std::vector<std::byte>, std::vector<Shader_input_element>>
   {
      auto shader = at_if(vertex_flags);

      if (!shader) {
         log_and_terminate("Attempt to get nonexistent vertex shader variation."sv);
      }

      return std::move(*shader);
   }

   auto at(const Pixel_shader_flags pixel_flags = {}) const noexcept
      -> Com_ptr<ID3D11PixelShader>
   {
      auto ps = at_if(pixel_flags);

      if (!ps) {
         log_and_terminate("Attempt to get nonexistent pixel shader variation."sv);
      }

      return Com_ptr{ps};
   }

   auto at_if(const Vertex_shader_flags vertex_flags = {}) const noexcept
      -> std::optional<std::tuple<Com_ptr<ID3D11VertexShader>, std::vector<std::byte>, std::vector<Shader_input_element>>>
   {
      if (auto shader = _vertex_shader_entrypoint.get(_vertex_static_flags, vertex_flags);
          !shader) {
         return std::nullopt;
      }
      else {
         auto [vs, bytecode, layout] = *shader;

         vs->AddRef();

         return std::tuple{Com_ptr{vs}, make_vector(bytecode), make_vector(layout)};
      }
   }

   auto at_if(const Pixel_shader_flags pixel_flags = {}) const noexcept
      -> Com_ptr<ID3D11PixelShader>
   {
      ID3D11PixelShader* ps = nullptr;

      if (ps = _pixel_shader_entrypoint.get(_pixel_static_flags, pixel_flags); !ps) {
         return nullptr;
      }
      ps->AddRef();

      return Com_ptr{ps};
   }

private:
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
