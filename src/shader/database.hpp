#pragma once

#include "bytecode_blob.hpp"
#include "com_ptr.hpp"
#include "common.hpp"
#include "rendertype_state_description.hpp"
#include "small_function.hpp"
#include "vertex_input_layout.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

#include <absl/container/flat_hash_map.h>

#include <d3d11_4.h>

namespace sp::shader {

class Database_internal;

template<typename Type>
class Group;

class Group_vertex;

using Group_compute = Group<ID3D11ComputeShader>;
using Group_hull = Group<ID3D11HullShader>;
using Group_domain = Group<ID3D11DomainShader>;
using Group_geometry = Group<ID3D11GeometryShader>;
using Group_pixel = Group<ID3D11PixelShader>;

template<typename T>
concept Static_shader_flags = std::is_integral_v<T> || std::is_enum_v<T>;

template<typename T>
concept State_vertex_callback =
   std::is_invocable_v<T, Vertex_shader_flags, Com_ptr<ID3D11VertexShader>, Bytecode_blob, Vertex_input_layout>;

template<typename T>
constexpr auto to_stage() noexcept -> Stage
{
   if (std::is_same_v<T, ID3D11ComputeShader>) return Stage::compute;
   if (std::is_same_v<T, ID3D11VertexShader>) return Stage::vertex;
   if (std::is_same_v<T, ID3D11HullShader>) return Stage::hull;
   if (std::is_same_v<T, ID3D11DomainShader>) return Stage::domain;
   if (std::is_same_v<T, ID3D11GeometryShader>) return Stage::geometry;
   if (std::is_same_v<T, ID3D11PixelShader>) return Stage::pixel;
}

struct Database_file_paths {
   std::filesystem::path shader_cache;
   std::filesystem::path shader_definitions;
   std::filesystem::path shader_source_files;
};

class Database {
public:
   Database(Com_ptr<ID3D11Device5> device, Database_file_paths file_paths) noexcept;

   ~Database();

   Database(const Database&) = delete;
   auto operator=(const Database&) -> Database& = delete;

   Database(Database&&) = delete;
   auto operator=(Database&&) -> Database& = delete;

   auto compute(const std::string_view name) noexcept -> Group_compute&;

   auto vertex(const std::string_view name) noexcept -> Group_vertex&;

   auto hull(const std::string_view name) noexcept -> Group_hull&;

   auto domain(const std::string_view name) noexcept -> Group_domain&;

   auto geometry(const std::string_view name) noexcept -> Group_geometry&;

   auto pixel(const std::string_view name) noexcept -> Group_pixel&;

   void cache_update() noexcept;

   void force_cache_save_to_disk() noexcept;

   auto internal() noexcept -> Database_internal&;

private:
   const std::unique_ptr<Database_internal> _database;

   absl::flat_hash_map<std::string, std::unique_ptr<Group_compute>> _groups_compute;
   absl::flat_hash_map<std::string, std::unique_ptr<Group_vertex>> _groups_vertex;
   absl::flat_hash_map<std::string, std::unique_ptr<Group_hull>> _groups_hull;
   absl::flat_hash_map<std::string, std::unique_ptr<Group_domain>> _groups_domain;
   absl::flat_hash_map<std::string, std::unique_ptr<Group_geometry>> _groups_geometry;
   absl::flat_hash_map<std::string, std::unique_ptr<Group_pixel>> _groups_pixel;
};

class Group_base {
public:
   Group_base(std::string name, Database_internal& database);

protected:
   auto entrypoint(const Stage stage, const std::string_view entrypoint_name,
                   const std::uint64_t static_flags) noexcept -> Com_ptr<IUnknown>;

   const std::string _name;

   Database_internal& _database;
};

template<typename T>
class Group : Group_base {
public:
   using Group_base::Group_base;

   Group(const Group&) = delete;
   auto operator=(const Group&) -> Group& = delete;

   Group(Group&&) = delete;
   auto operator=(Group&&) -> Group& = delete;

   using shader_interface = T;

   constexpr static auto shader_stage = to_stage<T>();

   template<Static_shader_flags Flags>
   auto entrypoint(const std::string_view name, const Flags static_flags = {}) noexcept
      -> Com_ptr<T>
   {
      return entrypoint(name, static_cast<std::uint64_t>(static_flags));
   }

   auto entrypoint(const std::string_view name,
                   const std::uint64_t static_flags = 0) noexcept -> Com_ptr<T>
   {
      auto abstract_shader = Group_base::entrypoint(shader_stage, name, static_flags);

      Com_ptr<T> shader;

      abstract_shader->QueryInterface(shader.clear_and_assign());

      return shader;
   }

   auto name() const noexcept -> std::string_view
   {
      return _name;
   }
};

class Group_vertex {
public:
   Group_vertex(std::string name, Database_internal& database);

   Group_vertex(const Group_vertex&) = delete;
   auto operator=(const Group_vertex&) -> Group_vertex& = delete;

   Group_vertex(Group_vertex&&) = delete;
   auto operator=(Group_vertex&&) -> Group_vertex& = delete;

   using shader_interface = ID3D11VertexShader;

   template<Static_shader_flags Flags>
   auto entrypoint(const std::string_view entrypoint_name,
                   const Flags static_flags = {},
                   const Vertex_shader_flags game_flags = {}) noexcept
      -> std::tuple<Com_ptr<ID3D11VertexShader>, Bytecode_blob, Vertex_input_layout>
   {
      return entrypoint(entrypoint_name,
                        static_cast<std::uint64_t>(static_flags), game_flags);
   }

   auto entrypoint(const std::string_view entrypoint_name,
                   const std::uint64_t static_flags = 0,
                   const Vertex_shader_flags game_flags = {}) noexcept
      -> std::tuple<Com_ptr<ID3D11VertexShader>, Bytecode_blob, Vertex_input_layout>;

   auto name() const noexcept -> std::string_view
   {
      return _name;
   }

private:
   const std::string _name;

   Database_internal& _database;
};

class Rendertype;
class Rendertype_state;

class Rendertypes_database {
public:
   Rendertypes_database(Database& database, const bool oit_capable) noexcept;

   Rendertypes_database(const Rendertypes_database&) = delete;
   auto operator=(const Rendertypes_database&) -> Rendertypes_database& = delete;

   Rendertypes_database(Rendertypes_database&&) = delete;
   auto operator=(Rendertypes_database&&) -> Rendertypes_database& = delete;

   auto operator[](const std::string_view rendertype) noexcept -> Rendertype&;

private:
   absl::flat_hash_map<std::string, std::unique_ptr<Rendertype>> _rendertypes;
};

class Rendertype {
private:
   using Container =
      absl::flat_hash_map<std::string, std::unique_ptr<Rendertype_state>>;

public:
   using iterator = Container::iterator;
   using const_iterator = Container::const_iterator;

   Rendertype(Database_internal& database,
              const absl::flat_hash_map<std::string, Rendertype_state_description>& states,
              std::string name, const bool oit_capable);

   Rendertype(const Rendertype&) = delete;
   auto operator=(const Rendertype&) -> Rendertype& = delete;

   Rendertype(Rendertype&&) = delete;
   auto operator=(Rendertype&&) -> Rendertype& = delete;

   auto state(const std::string_view state) noexcept -> Rendertype_state&;

   auto begin() noexcept -> iterator
   {
      return _states.begin();
   }

   auto end() noexcept -> iterator
   {
      return _states.end();
   }

   auto begin() const noexcept -> const_iterator
   {
      return _states.begin();
   }

   auto end() const noexcept -> const_iterator
   {
      return _states.end();
   }

   auto cbegin() const noexcept -> const_iterator
   {
      return begin();
   }

   auto cend() const noexcept -> const_iterator
   {
      return end();
   }

private:
   Container _states;
   const std::string _name;
};

class Rendertype_state {
public:
   Rendertype_state(Database_internal& database,
                    Rendertype_state_description description, const bool oit_capable);

   Rendertype_state(const Rendertype_state&) = delete;
   auto operator=(const Rendertype_state&) -> Rendertype_state& = delete;

   Rendertype_state(Rendertype_state&&) = delete;
   auto operator=(Rendertype_state&&) -> Rendertype_state& = delete;

   auto vertex(const Vertex_shader_flags game_flags) noexcept
      -> std::tuple<Com_ptr<ID3D11VertexShader>, Bytecode_blob, Vertex_input_layout>;

   auto pixel() noexcept -> Com_ptr<ID3D11PixelShader>;

   auto pixel_oit() noexcept -> Com_ptr<ID3D11PixelShader>;

   auto vertex(const Vertex_shader_flags game_flags,
               std::span<const std::string> extra_flags) noexcept
      -> std::tuple<Com_ptr<ID3D11VertexShader>, Bytecode_blob, Vertex_input_layout>;

   template<State_vertex_callback Callback>
   void vertex_copy_all(Callback&& callback, std::span<const std::string> extra_flags) noexcept
   {
      eval_vertex_shader_variations([&](const Vertex_shader_flags flags) {
         auto [shader, bytecode, input_layout] = vertex(flags, extra_flags);

         callback(flags, std::move(shader), std::move(bytecode),
                  std::move(input_layout));
      });
   }

   auto pixel(std::span<const std::string> extra_flags) noexcept
      -> Com_ptr<ID3D11PixelShader>;

   auto pixel_oit(std::span<const std::string> extra_flags) noexcept
      -> Com_ptr<ID3D11PixelShader>;

private:
   bool vertex_shader_supported(const Vertex_shader_flags game_flags) const noexcept;

   template<typename Callback>
   void eval_vertex_shader_variations(Callback&& callback) const noexcept
   {
      auto base_flags = Vertex_shader_flags::none;

      const auto input_state = _desc.vs_input_state;

      if (input_state.position) {
         base_flags |= Vertex_shader_flags::position;
      }

      if (input_state.normal) {
         base_flags |= Vertex_shader_flags::normal;
      }

      if (input_state.tangents && !input_state.tangents_unflagged) {
         base_flags |= Vertex_shader_flags::tangents;
      }

      if (input_state.texture_coords) {
         base_flags |= Vertex_shader_flags::texcoords;
      }

      callback(base_flags);

      if (input_state.skinned) {
         callback(base_flags | Vertex_shader_flags::hard_skinned);
      }

      if (input_state.color) {
         callback(base_flags | Vertex_shader_flags::color);
      }

      if (input_state.skinned && input_state.color) {
         callback(base_flags | Vertex_shader_flags::hard_skinned |
                  Vertex_shader_flags::color);
      }
   }

   static auto eval_static_flags(const std::span<const std::string> flag_names,
                                 const std::span<const std::string> set_flags) noexcept
      -> std::uint64_t;

   Database_internal& _database;

   const Rendertype_state_description _desc;

   const bool _oit_capable;
};

}
