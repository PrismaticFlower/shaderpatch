
#include "database.hpp"
#include "../logger.hpp"
#include "cache.hpp"
#include "compiler.hpp"
#include "entrypoint_description.hpp"
#include "group_definition.hpp"
#include "source_file_dependency_index.hpp"
#include "source_file_store.hpp"

#include <algorithm>
#include <bitset>
#include <chrono>
#include <future>
#include <ranges>

#include <absl/container/flat_hash_map.h>

#include <comdef.h>

using namespace std::literals;

namespace sp::shader {

namespace {

template<typename T>
auto get_shader_group(const absl::flat_hash_map<std::string, std::unique_ptr<T>>& groups,
                      const std::string_view name) noexcept -> T&
{
   if (auto group = groups.find(name); group != groups.end()) {
      return *group->second;
   }

   log_and_terminate("Unable to find shader group '"sv, name, "'"sv);
}

template<typename T>
auto create_shader(ID3D11Device5& device, const Bytecode_blob& bytecode) noexcept
   -> Com_ptr<T>
{
   constexpr auto create = [] {
      if constexpr (std::is_same_v<T, ID3D11ComputeShader>) {
         return &ID3D11Device5::CreateComputeShader;
      }
      else if constexpr (std::is_same_v<T, ID3D11VertexShader>) {
         return &ID3D11Device5::CreateVertexShader;
      }
      else if constexpr (std::is_same_v<T, ID3D11HullShader>) {
         return &ID3D11Device5::CreateHullShader;
      }
      else if constexpr (std::is_same_v<T, ID3D11DomainShader>) {
         return &ID3D11Device5::CreateDomainShader;
      }
      else if constexpr (std::is_same_v<T, ID3D11GeometryShader>) {
         return &ID3D11Device5::CreateGeometryShader;
      }
      else if constexpr (std::is_same_v<T, ID3D11PixelShader>) {
         return &ID3D11Device5::CreatePixelShader;
      }
   }();

   Com_ptr<T> shader;

   if (auto result = std::invoke(create, device, bytecode.data(), bytecode.size(),
                                 nullptr, shader.clear_and_assign());
       FAILED(result)) {
      log(Log_level::warning, "Failed to create shader, reason: "sv,
          _com_error{result}.ErrorMessage());
   }

   return shader;
}

auto create_entrypoint_vertex_state(const Group_definition& definition,
                                    const Group_definition::Entrypoint& entrypoint) noexcept
   -> Entrypoint_vertex_state
{
   const bool use_custom_input_layout =
      entrypoint.vertex_state.input_layout != "$auto"sv;

   if (use_custom_input_layout) {

      if (auto it = definition.input_layouts.find(entrypoint.vertex_state.input_layout);
          it != definition.input_layouts.end()) {
         return {.use_custom_input_layout = use_custom_input_layout,
                 .custom_input_layout = it->second,
                 .generic_input_state = entrypoint.vertex_state.generic_input};
      }
      else {
         log_and_terminate("Unable to find vertex input layout '"sv,
                           entrypoint.vertex_state.input_layout,
                           "' in shader group '"sv, definition.group_name, "'"sv);
      }
   }
   else {
      return {.use_custom_input_layout = use_custom_input_layout,
              .generic_input_state = entrypoint.vertex_state.generic_input};
   }
}

auto create_entrypoint_descs(const Group_definition& definition) noexcept
   -> absl::flat_hash_map<std::string, Entrypoint_description>
{
   absl::flat_hash_map<std::string, Entrypoint_description> entrypoints;

   entrypoints.reserve(definition.entrypoints.size());

   for (const auto& [name, entrypoint] : definition.entrypoints) {
      entrypoints.emplace(name,
                          Entrypoint_description{
                             .function_name = entrypoint.function_name.value_or(name),
                             .source_name = definition.source_name,
                             .stage = entrypoint.stage,
                             .vertex_state =
                                entrypoint.stage == Stage::vertex
                                   ? create_entrypoint_vertex_state(definition, entrypoint)
                                   : Entrypoint_vertex_state{},
                             .static_flags = Static_flags{entrypoint.static_flags},
                             .preprocessor_defines = entrypoint.preprocessor_defines});
   }

   return entrypoints;
}

auto eval_rendertype_state_static_flags(
   const std::vector<std::string>& static_flags,
   const absl::flat_hash_map<std::string, bool>& rendertype_static_flags,
   const absl::flat_hash_map<std::string, bool>& state_static_flags) -> std::uint64_t
{
   std::bitset<Static_flags::max_flags> flags{};

   const std::size_t flag_count = std::min(static_flags.size(), flags.size());

   for (std::size_t i = 0u; i < flag_count; ++i) {
      if (auto it = rendertype_static_flags.find(static_flags[i]);
          it != rendertype_static_flags.end()) {
         flags[i] = it->second;
      }

      if (auto it = state_static_flags.find(static_flags[i]);
          it != state_static_flags.end()) {
         flags[i] = it->second;
      }
   }

   return flags.to_ullong();
}

auto create_rendertype_state_descs(const Group_definition& definition,
                                   const Group_definition::Rendertype& rendertype) noexcept
   -> absl::flat_hash_map<std::string, Rendertype_state_description>
{
   absl::flat_hash_map<std::string, Rendertype_state_description> states;

   for (const auto& [state_name, state] : definition.states) {

      const auto verify_entrypoint_exists = [&](const std::string& entrypoint) {
         if (definition.entrypoints.contains(entrypoint)) return;

         log_and_terminate("Can't find entrypoint definition '"sv, entrypoint,
                           "' in shader group '"sv, definition.group_name, "'"sv);
      };

      verify_entrypoint_exists(state.vs_entrypoint);
      verify_entrypoint_exists(state.ps_entrypoint);

      if (state.ps_oit_entrypoint) {
         verify_entrypoint_exists(*state.ps_oit_entrypoint);
      }

      if (state.hs_entrypoint) {
         verify_entrypoint_exists(*state.hs_entrypoint);
      }

      if (state.ds_entrypoint) {
         verify_entrypoint_exists(*state.ds_entrypoint);
      }

      if (state.gs_entrypoint) {
         verify_entrypoint_exists(*state.gs_entrypoint);
      }

      states.emplace(
         state_name,
         Rendertype_state_description{
            .group_name = definition.group_name,

            .vs_entrypoint = state.vs_entrypoint,
            .vs_static_flags = eval_rendertype_state_static_flags(
               definition.entrypoints.at(state.vs_entrypoint).static_flags,
               rendertype.static_flags, state.vs_static_flags),
            .vs_input_state =
               definition.entrypoints.at(state.vs_entrypoint).vertex_state.generic_input,

            .ps_entrypoint = state.ps_entrypoint,
            .ps_static_flags = eval_rendertype_state_static_flags(
               definition.entrypoints.at(state.ps_entrypoint).static_flags,
               rendertype.static_flags, state.ps_static_flags),

            .ps_oit_entrypoint = state.ps_oit_entrypoint,
            .ps_oit_static_flags =
               state.ps_oit_entrypoint
                  ? eval_rendertype_state_static_flags(
                       definition.entrypoints.at(*state.ps_oit_entrypoint).static_flags,
                       rendertype.static_flags, state.ps_oit_static_flags)
                  : 0,

            .hs_entrypoint = state.hs_entrypoint,
            .hs_static_flags =
               state.hs_entrypoint
                  ? eval_rendertype_state_static_flags(
                       definition.entrypoints.at(*state.hs_entrypoint).static_flags,
                       rendertype.static_flags, state.hs_static_flags)
                  : 0,

            .ds_entrypoint = state.ds_entrypoint,
            .ds_static_flags =
               state.ds_entrypoint
                  ? eval_rendertype_state_static_flags(
                       definition.entrypoints.at(*state.ds_entrypoint).static_flags,
                       rendertype.static_flags, state.ds_static_flags)
                  : 0,

            .gs_entrypoint = state.gs_entrypoint,
            .gs_static_flags =
               state.gs_entrypoint
                  ? eval_rendertype_state_static_flags(
                       definition.entrypoints.at(*state.gs_entrypoint).static_flags,
                       rendertype.static_flags, state.gs_static_flags)
                  : 0});
   }

   return states;
}

class Cache_disk_updater {
public:
   constexpr static auto min_update_interval = 1min;

   void mark_dirty() noexcept
   {
      _disk_cache_dirty = true;
   }

   bool should_update() const noexcept
   {
      if (!_disk_cache_dirty) return false;

      return (std::chrono::steady_clock::now() - _last_update) >= min_update_interval;
   }

   void update(Cache& cache, const std::filesystem::path& cache_path) noexcept
   {
      _update_future = std::async(std::launch::async, [&cache, cache_path] {
         cache.save_to_file(cache_path);
      });
      _disk_cache_dirty = false;
      _last_update = std::chrono::steady_clock::now();
   }

private:
   bool _disk_cache_dirty = false;
   std::chrono::steady_clock::time_point _last_update =
      std::chrono::steady_clock::now();
   std::future<void> _update_future;
};

}

class Database_internal {
public:
   Database_internal(Com_ptr<ID3D11Device5> device, Database_file_paths file_paths)
      : _device{device}, _file_paths{std::move(file_paths)}
   {
      const auto definitions = load_group_definitions(_file_paths.shader_definitions);

      using std::views::filter;

      for (const auto& definition :
           definitions | filter([](const Group_definition& definition) {
              return !definition.entrypoints.empty();
           })) {
         _entrypoint_descs.emplace(definition.group_name,
                                   create_entrypoint_descs(definition));
      }

      for (const auto& definition :
           definitions | filter([](const Group_definition& definition) {
              return !definition.rendertypes.empty() && !definition.states.empty();
           })) {
         for (const auto& [rendertype_name, rendertype_def] : definition.rendertypes) {
            _rendertypes_states.emplace(rendertype_name,
                                        create_rendertype_state_descs(definition,
                                                                      rendertype_def));
         }
      }

      _cache.clear_stale_entries(_source_dependency_index, _source_file_store,
                                 definitions);
   }

   template<typename T>
   auto get(const std::string_view group_name, const std::string_view entrypoint_name,
            const std::uint64_t static_flags) noexcept -> Com_ptr<T>
   {
      if (auto cached = _cache.get_if<T>(group_name, entrypoint_name, static_flags);
          cached) {
         return cached->shader;
      }

      // Multithreading stuff to stop double compilation goes here.

      const auto& entrypoint_desc = get_entrypoint_desc(group_name, entrypoint_name);

      if (entrypoint_desc.stage != to_stage<T>()) {
         log_and_terminate("Shader stage mismatch for entrypoint '"sv,
                           entrypoint_name, "' from group '"sv, group_name, "'"sv);
      }

      auto bytecode = compile(_source_file_store, entrypoint_desc, static_flags);

      auto shader = create_shader<T>(*_device, bytecode);

      if (!shader) {
         log_and_terminate("Unable to recover from failed shader creation!"sv);
      }

      _cache.add<T>(group_name, entrypoint_name, static_flags,
                    {.shader = shader, .bytecode = bytecode});
      _cache_disk_updater.mark_dirty();

      return shader;
   }

   auto get_vs(const std::string_view group_name,
               const std::string_view entrypoint_name, const std::uint64_t static_flags,
               const Vertex_shader_flags game_flags) noexcept
      -> std::tuple<Com_ptr<ID3D11VertexShader>, Bytecode_blob, Vertex_input_layout>
   {
      const auto& entrypoint_desc = get_entrypoint_desc(group_name, entrypoint_name);

      auto vertex_input_layout =
         entrypoint_desc.vertex_state.use_custom_input_layout
            ? Vertex_input_layout{entrypoint_desc.vertex_state
                                     .custom_input_layout.begin(),
                                  entrypoint_desc.vertex_state
                                     .custom_input_layout.end()}
            : get_vertex_input_layout(entrypoint_desc.vertex_state.generic_input_state,
                                      game_flags);

      if (auto cached = _cache.get_vs_if(group_name, entrypoint_name,
                                         static_flags, game_flags);
          cached) {
         return {cached->shader, cached->bytecode, std::move(vertex_input_layout)};
      }

      // Multithreading stuff to stop double compilation goes here.

      if (entrypoint_desc.stage != Stage::vertex) {
         log_and_terminate("Shader stage mismatch for entrypoint '"sv,
                           entrypoint_name, "' from group '"sv, group_name, "'"sv);
      }

      auto bytecode =
         compile(_source_file_store, entrypoint_desc, static_flags, game_flags);

      auto shader = create_shader<ID3D11VertexShader>(*_device, bytecode);

      if (!shader) {
         log_and_terminate("Unable to recover from failed shader creation!"sv);
      }

      _cache.add_vs(group_name, entrypoint_name, static_flags, game_flags,
                    {.shader = shader, .bytecode = bytecode});
      _cache_disk_updater.mark_dirty();

      return {shader, bytecode, std::move(vertex_input_layout)};
   }

   void cache_update() noexcept
   {
      if (_cache_disk_updater.should_update()) {
         _cache_disk_updater.update(_cache, _file_paths.shader_cache);
      }
   }

   void force_cache_save_to_disk() noexcept
   {
      _cache.save_to_file(_file_paths.shader_cache);
   }

   template<typename T>
   auto get_shader_groups() noexcept
      -> absl::flat_hash_map<std::string, std::unique_ptr<T>>
   {
      absl::flat_hash_map<std::string, std::unique_ptr<T>> groups;

      for (const auto& [group, entrypoints] : _entrypoint_descs) {
         for (const auto& [entrypoint_name, entrypoint] : entrypoints) {
            if (entrypoint.stage == to_stage<T::shader_interface>()) {
               groups.emplace(group, std::make_unique<T>(group, *this));

               break;
            }
         }
      }

      return groups;
   }

   auto get_shader_rendertypes_states() const noexcept
      -> const absl::flat_hash_map<std::string, absl::flat_hash_map<std::string, Rendertype_state_description>>&
   {
      return _rendertypes_states;
   }

private:
   auto get_entrypoint_desc(const std::string_view group_name,
                            const std::string_view entrypoint_name) const noexcept
      -> const Entrypoint_description&

   {
      if (auto group = _entrypoint_descs.find(group_name);
          group != _entrypoint_descs.end()) {
         if (auto entrypoint = group->second.find(entrypoint_name);
             entrypoint != group->second.end()) {
            return entrypoint->second;
         }

         log_and_terminate("Unable to find shader entrypoint '"sv, entrypoint_name,
                           "' in shader group '"sv, group_name, "'!"sv);
      }

      log_and_terminate("Unable to find shader group '"sv, group_name, "'!"sv);
   }

   Com_ptr<ID3D11Device5> _device;
   const Database_file_paths _file_paths;
   Cache _cache{*_device, _file_paths.shader_cache};
   Cache_disk_updater _cache_disk_updater;
   Source_file_store _source_file_store{_file_paths.shader_source_files};
   Source_file_dependency_index _source_dependency_index{_source_file_store};

   absl::flat_hash_map<std::string, absl::flat_hash_map<std::string, Entrypoint_description>> _entrypoint_descs;
   absl::flat_hash_map<std::string, absl::flat_hash_map<std::string, Rendertype_state_description>> _rendertypes_states;
};

Database::Database(Com_ptr<ID3D11Device5> device, Database_file_paths file_paths) noexcept
   : _database{std::make_unique<Database_internal>(device, std::move(file_paths))}
{
   _groups_compute = _database->get_shader_groups<Group_compute>();
   _groups_vertex = _database->get_shader_groups<Group_vertex>();
   _groups_hull = _database->get_shader_groups<Group_hull>();
   _groups_domain = _database->get_shader_groups<Group_domain>();
   _groups_geometry = _database->get_shader_groups<Group_geometry>();
   _groups_pixel = _database->get_shader_groups<Group_pixel>();
}

Database::~Database() = default;

auto Database::compute(const std::string_view name) noexcept -> Group_compute&
{
   return get_shader_group(_groups_compute, name);
}

auto Database::vertex(const std::string_view name) noexcept -> Group_vertex&
{
   return get_shader_group(_groups_vertex, name);
}

auto Database::hull(const std::string_view name) noexcept -> Group_hull&
{
   return get_shader_group(_groups_hull, name);
}

auto Database::domain(const std::string_view name) noexcept -> Group_domain&
{
   return get_shader_group(_groups_domain, name);
}

auto Database::geometry(const std::string_view name) noexcept -> Group_geometry&
{
   return get_shader_group(_groups_geometry, name);
}

auto Database::pixel(const std::string_view name) noexcept -> Group_pixel&
{
   return get_shader_group(_groups_pixel, name);
}

void Database::cache_update() noexcept
{
   _database->cache_update();
}

void Database::force_cache_save_to_disk() noexcept
{
   _database->force_cache_save_to_disk();
}

auto Database::internal() noexcept -> Database_internal&
{
   return *_database;
}

Group_base::Group_base(std::string name, Database_internal& database)
   : _name{std::move(name)}, _database{database}
{
}

auto Group_base::entrypoint(const Stage stage, const std::string_view entrypoint_name,
                            const std::uint64_t static_flags) noexcept
   -> Com_ptr<IUnknown>
{
   switch (stage) {
   case Stage::compute:
      return _database.get<ID3D11ComputeShader>(_name, entrypoint_name, static_flags);
   case Stage::hull:
      return _database.get<ID3D11HullShader>(_name, entrypoint_name, static_flags);
   case Stage::domain:
      return _database.get<ID3D11DomainShader>(_name, entrypoint_name, static_flags);
   case Stage::geometry:
      return _database.get<ID3D11GeometryShader>(_name, entrypoint_name, static_flags);
   case Stage::pixel:
      return _database.get<ID3D11PixelShader>(_name, entrypoint_name, static_flags);
   default:
      std::terminate();
   }
}

Group_vertex::Group_vertex(std::string name, Database_internal& database)
   : _name{std::move(name)}, _database{database}
{
}

auto Group_vertex::entrypoint(const std::string_view entrypoint_name,
                              const std::uint64_t static_flags,
                              const Vertex_shader_flags game_flags) noexcept
   -> std::tuple<Com_ptr<ID3D11VertexShader>, Bytecode_blob, Vertex_input_layout>
{
   return _database.get_vs(_name, entrypoint_name, static_flags, game_flags);
}

Rendertypes_database::Rendertypes_database(Database& database, const bool oit_capable) noexcept
{
   const auto& rendertypes_states =
      database.internal().get_shader_rendertypes_states();

   _rendertypes.reserve(rendertypes_states.size());

   for (const auto& [rendertype, states] : rendertypes_states) {
      _rendertypes[rendertype] =
         std::make_unique<Rendertype>(database.internal(), states, rendertype,
                                      oit_capable);
   }
}

auto Rendertypes_database::operator[](const std::string_view rendertype) noexcept
   -> Rendertype&
{
   if (auto it = _rendertypes.find(rendertype); it != _rendertypes.end()) {
      return *it->second;
   }

   log_and_terminate("Attempt to get nonexistent rendertype '"sv, rendertype, "'!"sv);
}

Rendertype::Rendertype(Database_internal& database,
                       const absl::flat_hash_map<std::string, Rendertype_state_description>& states,
                       std::string name, const bool oit_capable)
   : _name{std::move(name)}
{
   _states.reserve(states.size());

   for (const auto& [state, desc] : states) {
      _states[state] = std::make_unique<Rendertype_state>(database, desc, oit_capable);
   }
}

auto Rendertype::state(const std::string_view state) noexcept -> Rendertype_state&
{
   if (auto it = _states.find(state); it != _states.end()) {
      return *it->second;
   }

   log_and_terminate("Attempt to get nonexistent state '"sv, state,
                     "' from rendertype'"sv, _name, "'!"sv);
}

Rendertype_state::Rendertype_state(Database_internal& database,
                                   Rendertype_state_description description,
                                   const bool oit_capable)
   : _database{database}, _desc{std::move(description)}, _oit_capable{oit_capable}
{
}

auto Rendertype_state::vertex(const Vertex_shader_flags game_flags) noexcept
   -> std::tuple<Com_ptr<ID3D11VertexShader>, Bytecode_blob, Vertex_input_layout>
{
   if (!vertex_shader_supported(game_flags)) return {nullptr, {}, {}};

   return _database.get_vs(_desc.group_name, _desc.vs_entrypoint,
                           _desc.vs_static_flags, game_flags);
}

auto Rendertype_state::pixel() noexcept -> Com_ptr<ID3D11PixelShader>
{
   return _database.get<ID3D11PixelShader>(_desc.group_name, _desc.ps_entrypoint,
                                           _desc.ps_static_flags);
}

auto Rendertype_state::pixel_oit() noexcept -> Com_ptr<ID3D11PixelShader>
{
   if (!_oit_capable || !_desc.ps_oit_entrypoint) return nullptr;

   return _database.get<ID3D11PixelShader>(_desc.group_name, *_desc.ps_oit_entrypoint,
                                           _desc.ps_oit_static_flags);
}

auto Rendertype_state::hull() noexcept -> Com_ptr<ID3D11HullShader>
{
   if (!_desc.hs_entrypoint) return nullptr;

   return _database.get<ID3D11HullShader>(_desc.group_name, *_desc.hs_entrypoint,
                                          _desc.hs_static_flags);
}

auto Rendertype_state::domain() noexcept -> Com_ptr<ID3D11DomainShader>
{
   if (!_desc.ds_entrypoint) return nullptr;

   return _database.get<ID3D11DomainShader>(_desc.group_name, *_desc.ds_entrypoint,
                                            _desc.ds_static_flags);
}

auto Rendertype_state::geometry() noexcept -> Com_ptr<ID3D11GeometryShader>
{
   if (!_desc.gs_entrypoint) return nullptr;

   return _database.get<ID3D11GeometryShader>(_desc.group_name, *_desc.gs_entrypoint,
                                              _desc.gs_static_flags);
}

bool Rendertype_state::vertex_shader_supported(const Vertex_shader_flags game_flags) const noexcept
{
   const auto input_state = _desc.vs_input_state;

   if ((game_flags & Vertex_shader_flags::compressed) != Vertex_shader_flags::none &&
       !(input_state.dynamic_compression || input_state.always_compressed)) {
      return false;
   }

   if ((game_flags & Vertex_shader_flags::position) != Vertex_shader_flags::none &&
       !input_state.position) {
      return false;
   }

   if ((game_flags & Vertex_shader_flags::normal) != Vertex_shader_flags::none &&
       !input_state.normal) {
      return false;
   }

   if ((game_flags & Vertex_shader_flags::tangents) != Vertex_shader_flags::none &&
       !input_state.tangents) {
      return false;
   }

   if ((game_flags & Vertex_shader_flags::texcoords) != Vertex_shader_flags::none &&
       !input_state.texture_coords) {
      return false;
   }

   if ((game_flags & Vertex_shader_flags::color) != Vertex_shader_flags::none &&
       !input_state.color) {
      return false;
   }

   if ((game_flags & Vertex_shader_flags::hard_skinned) != Vertex_shader_flags::none &&
       !input_state.skinned) {
      return false;
   }

   return true;
}
}
