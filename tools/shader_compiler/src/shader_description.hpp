
#include "shader_entrypoint.hpp"
#include "shader_input_element.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

namespace sp::shader {

struct State {
   std::string vs_entrypoint;
   std::unordered_map<std::string, bool> vs_static_flag_values;

   std::string ps_entrypoint;
   std::unordered_map<std::string, bool> ps_static_flag_values;

   std::optional<std::string> ps_oit_entrypoint;
   std::unordered_map<std::string, bool> ps_oit_static_flag_values;

   std::optional<std::string> hs_entrypoint;
   std::unordered_map<std::string, bool> hs_static_flag_values;

   std::optional<std::string> ds_entrypoint;
   std::unordered_map<std::string, bool> ds_static_flag_values;

   std::optional<std::string> gs_entrypoint;
   std::unordered_map<std::string, bool> gs_static_flag_values;
};

struct Rendertype {
   std::unordered_map<std::string, State> states;
};

struct Description {
   std::string group_name;
   std::string source_name;

   std::unordered_map<std::string, std::vector<Input_element>> input_layouts;
   std::unordered_map<std::string, Entrypoint> entrypoints;
   std::unordered_map<std::string, Rendertype> rendertypes;
};

auto read_json_rendertype(const nlohmann::json& j, const nlohmann::json& j_states,
                          const std::unordered_map<std::string, Entrypoint>& entrypoints)
   -> Rendertype;

auto read_json_shader_state(const nlohmann::json& j,
                            const nlohmann::json& j_rt_static_flags,
                            const std::unordered_map<std::string, Entrypoint>& entrypoints)
   -> State;

auto read_json_static_flag_values(const nlohmann::json& j,
                                  const nlohmann::json& j_rt_static_flags,
                                  std::span<const std::string> flags)
   -> std::unordered_map<std::string, bool>;

inline void from_json(const nlohmann::json& j, Description& description)
{
   using namespace std::literals;

   description.group_name = j.at("group_name"s).get<std::string>();
   description.source_name = j.at("source_name"s).get<std::string>();

   description.input_layouts = j.value("input_layouts"s, description.input_layouts);

   description.entrypoints =
      j.at("entrypoints"s).get<decltype(description.entrypoints)>();

   const auto j_rendertypes = j.at("rendertypes"s);
   const auto j_states = j.at("states"s);

   for (auto it = j_rendertypes.cbegin(); it != j_rendertypes.cend(); ++it) {
      if (description.rendertypes.count(it.key())) {
         throw compose_exception<std::runtime_error>("duplicate shader rendertype name \""s,
                                                     it.key(), "\"");
      }

      description.rendertypes[it.key()] =
         read_json_rendertype(*it, j_states, description.entrypoints);
   }
}

inline auto read_json_rendertype(const nlohmann::json& j, const nlohmann::json& j_states,
                                 const std::unordered_map<std::string, Entrypoint>& entrypoints)
   -> Rendertype
{
   using namespace std::literals;

   Rendertype rendertype;

   const auto j_rt_static_flags = j.value("static_flags"s, nlohmann::json::object());

   for (auto it = j_states.cbegin(); it != j_states.cend(); ++it) {
      if (rendertype.states.count(it.key())) {
         throw compose_exception<std::runtime_error>("duplicate shader state name \""s,
                                                     it.key(), "\"");
      }

      rendertype.states.emplace(it.key(), read_json_shader_state(*it, j_rt_static_flags,
                                                                 entrypoints));
   }

   return rendertype;
}

inline auto read_json_shader_state(
   const nlohmann::json& j, const nlohmann::json& j_rt_static_flags,
   const std::unordered_map<std::string, Entrypoint>& entrypoints) -> State
{
   using namespace std::literals;

   if (j.at("type"s) != "standard"s) {
      throw compose_exception<std::runtime_error>("shader state type is not \"standard\" was \""s,
                                                  j.at("type"s), "\"");
   }

   State state;

   auto j_vs = j.at("vertex_shader"s);

   state.vs_entrypoint = j_vs.at("entrypoint"s).get<std::string>();

   if (!entrypoints.count(state.vs_entrypoint)) {
      throw compose_exception<std::runtime_error>("no shader entrypoint named \""s,
                                                  state.vs_entrypoint, "\"");
   }

   state.vs_static_flag_values = read_json_static_flag_values(
      j_vs.value("static_flags"s, nlohmann::json::object()), j_rt_static_flags,
      entrypoints.at(state.vs_entrypoint).static_flags.list_flags());

   auto j_ps = j.at("pixel_shader"s);

   state.ps_entrypoint = j_ps.at("entrypoint"s).get<std::string>();

   if (!entrypoints.count(state.ps_entrypoint)) {
      throw compose_exception<std::runtime_error>("no shader entrypoint named \""s,
                                                  state.ps_entrypoint, "\"");
   }

   state.ps_static_flag_values = read_json_static_flag_values(
      j_ps.value("static_flags"s, nlohmann::json::object()), j_rt_static_flags,
      entrypoints.at(state.ps_entrypoint).static_flags.list_flags());

   if (const auto ps_oit = j.find("pixel_oit_shader"s); ps_oit != j.end()) {
      state.ps_oit_entrypoint = ps_oit->at("entrypoint"s).get<std::string>();

      if (!entrypoints.count(*state.ps_oit_entrypoint)) {
         throw compose_exception<std::runtime_error>("no shader entrypoint named \""s,
                                                     *state.ps_oit_entrypoint, "\"");
      }

      state.ps_oit_static_flag_values = read_json_static_flag_values(
         ps_oit->value("static_flags"s, nlohmann::json::object()), j_rt_static_flags,
         entrypoints.at(*state.ps_oit_entrypoint).static_flags.list_flags());
   }

   if (const auto oit_entrypoint = j_ps.find("oit_entrypoint"s);
       oit_entrypoint != j_ps.end()) {
      state.ps_oit_entrypoint = oit_entrypoint->get<std::string>();

      if (!entrypoints.count(*state.ps_oit_entrypoint)) {
         throw compose_exception<std::runtime_error>("no shader entrypoint named \""s,
                                                     *state.ps_oit_entrypoint, "\"");
      }
   }

   if (const auto hs = j.find("hull_shader"s); hs != j.end()) {
      state.hs_entrypoint = hs->at("entrypoint"s).get<std::string>();

      if (!entrypoints.count(*state.hs_entrypoint)) {
         throw compose_exception<std::runtime_error>("no shader entrypoint named \""s,
                                                     *state.hs_entrypoint, "\"");
      }

      state.hs_static_flag_values = read_json_static_flag_values(
         hs->value("static_flags"s, nlohmann::json::object()), j_rt_static_flags,
         entrypoints.at(*state.hs_entrypoint).static_flags.list_flags());
   }

   if (const auto ds = j.find("domain_shader"s); ds != j.end()) {
      state.ds_entrypoint = ds->at("entrypoint"s).get<std::string>();

      if (!entrypoints.count(*state.ds_entrypoint)) {
         throw compose_exception<std::runtime_error>("no shader entrypoint named \""s,
                                                     *state.ds_entrypoint, "\"");
      }

      state.ds_static_flag_values = read_json_static_flag_values(
         ds->value("static_flags"s, nlohmann::json::object()), j_rt_static_flags,
         entrypoints.at(*state.ds_entrypoint).static_flags.list_flags());
   }

   if (const auto gs = j.find("geometry_shader"s); gs != j.end()) {
      state.gs_entrypoint = gs->at("entrypoint"s).get<std::string>();

      if (!entrypoints.count(*state.gs_entrypoint)) {
         throw compose_exception<std::runtime_error>("no shader entrypoint named \""s,
                                                     *state.gs_entrypoint, "\"");
      }

      state.ds_static_flag_values = read_json_static_flag_values(
         gs->value("static_flags"s, nlohmann::json::object()), j_rt_static_flags,
         entrypoints.at(*state.gs_entrypoint).static_flags.list_flags());
   }

   return state;
}

inline auto read_json_static_flag_values(const nlohmann::json& j,
                                         const nlohmann::json& j_rt_static_flags,
                                         std::span<const std::string> flags)
   -> std::unordered_map<std::string, bool>
{
   using namespace std::literals;

   std::unordered_map<std::string, bool> set_flags;

   for (const auto& flag : flags) {
      if (const auto it = j.find(flag); it != j.cend()) {
         set_flags[flag] = it.value();
      }
      else {
         if (const auto rt_it = j_rt_static_flags.find(flag);
             rt_it != j_rt_static_flags.cend()) {
            set_flags[flag] = rt_it.value();
         }
         else {
            throw compose_exception<std::runtime_error>("static user flag is not set in shader state or shader rendertype type! Flag is \""s,
                                                        flag, "\"");
         }
      }
   }

   return set_flags;
}
}
