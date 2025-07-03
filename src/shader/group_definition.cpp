
#include "group_definition.hpp"
#include "../logger.hpp"
#include "file_helpers.hpp"
#include "retry_dialog.hpp"

#include <algorithm>
#include <fstream>
#include <ranges>
#include <stdexcept>

#include <nlohmann/json.hpp>

using namespace std::literals;

namespace sp::shader {

namespace {

template<typename T>
void get_to_if(const nlohmann::json& j, const std::string& key, T& v)
{
   if (auto it = j.find(key); it != j.end()) {
      it->get_to(v);
   }
}

template<typename T>
void get_to_if(const nlohmann::json& j, const std::string& key, std::optional<T>& v)
{
   if (auto it = j.find(key); it != j.end()) {
      it->get_to(v.emplace());
   }
   else {
      v = std::nullopt;
   }
}

}

void from_json(const nlohmann::json& j, Vertex_input_type& type)
{
   std::string str;

   j.get_to(str);

   // clang-format off
   if (str == "float4"sv)      type = Vertex_input_type::float4;
   else if (str == "float3"sv) type = Vertex_input_type::float3;
   else if (str == "float2"sv) type = Vertex_input_type::float2;
   else if (str == "float1"sv) type = Vertex_input_type::float1;
   else if (str == "uint4"sv)  type = Vertex_input_type::uint4;
   else if (str == "uint3"sv)  type = Vertex_input_type::uint3;
   else if (str == "uint2"sv)  type = Vertex_input_type::uint2;
   else if (str == "uint1"sv)  type = Vertex_input_type::uint1;
   else if (str == "sint4"sv)  type = Vertex_input_type::sint4;
   else if (str == "sint3"sv)  type = Vertex_input_type::sint3;
   else if (str == "sint2"sv)  type = Vertex_input_type::sint2;
   else if (str == "sint1"sv)  type = Vertex_input_type::sint1;
   else throw std::runtime_error{"bad shader stage"s};
   // clang-format on
}

void from_json(const nlohmann::json& j, Vertex_input_element& elem)
{
   j.at("semantic"s).at("name"s).get_to(elem.semantic_name);
   j.at("semantic"s).at("index"s).get_to(elem.semantic_index);
   j.at("format"s).get_to(elem.input_type);
}

void from_json(const nlohmann::json& j, Stage& stage)
{
   std::string str;

   j.get_to(str);

   // clang-format off
   if (str == "compute"s) stage = Stage::compute;
   else if (str == "vertex"s) stage = Stage::vertex;
   else if (str == "hull"s) stage = Stage::hull;
   else if (str == "domain"s) stage = Stage::domain;
   else if (str == "geometry"s) stage = Stage::geometry;
   else if (str == "pixel"s) stage = Stage::pixel;
   else throw std::runtime_error{"bad shader stage"s};
   // clang-format on
}

void from_json(const nlohmann::json& j, Preprocessor_define& define)
{
   j.at(0).get_to(define.name);
   j.at(1).get_to(define.definition);
}

void from_json(const nlohmann::json& j, Vertex_generic_input_state& input_state)
{
   const auto get_flag_if = [&](const std::string& key) {
      bool value = false;

      get_to_if(j, key, value);

      return value;
   };

   input_state.dynamic_compression = get_flag_if("dynamic_compression"s);
   input_state.always_compressed = get_flag_if("always_compressed"s);
   input_state.position = get_flag_if("position"s);
   input_state.skinned = get_flag_if("skinned"s);
   input_state.normal = get_flag_if("normal"s);
   input_state.tangents = get_flag_if("tangents"s);
   input_state.tangents_unflagged = get_flag_if("tangents_unflagged"s);
   input_state.color = get_flag_if("color"s);
   input_state.texture_coords = get_flag_if("texture_coords"s);
}

void from_json(const nlohmann::json& j,
               Group_definition::Entrypoint::Vertex_state& vertex_state)
{
   j.at("input_layout"s).get_to(vertex_state.input_layout);

   get_to_if(j, "generic_input"s, vertex_state.generic_input);
}

void from_json(const nlohmann::json& j, Group_definition::Entrypoint& entrypoint)
{
   j.at("stage"s).get_to(entrypoint.stage);

   get_to_if(j, "function_name"s, entrypoint.function_name);
   get_to_if(j, "vertex_state"s, entrypoint.vertex_state);
   get_to_if(j, "static_flags"s, entrypoint.static_flags);
   get_to_if(j, "defines"s, entrypoint.preprocessor_defines);
}

void from_json(const nlohmann::json& j, Group_definition::Rendertype& rendertype)
{
   get_to_if(j, "static_flags"s, rendertype.static_flags);
}

void from_json(const nlohmann::json& j, Group_definition::State& state)
{
   const auto read_state_entrypoint =
      [&](const std::string& name, std::string& entrypoint,
          absl::flat_hash_map<std::string, bool>& static_flags) {
         j.at(name).at("entrypoint"s).get_to(entrypoint);
         get_to_if(j.at(name), "static_flags"s, static_flags);
      };

   const auto read_state_ps_entrypoint =
      [&](const std::string& name, std::string& entrypoint,
          std::optional<std::string>& al_entrypoint,
          absl::flat_hash_map<std::string, bool>& static_flags) {
         j.at(name).at("entrypoint"s).get_to(entrypoint);
         get_to_if(j.at(name), "al_entrypoint"s, al_entrypoint);
         get_to_if(j.at(name), "static_flags"s, static_flags);
      };

   const auto read_state_entrypoint_optional =
      [&](const std::string& name, std::optional<std::string>& entrypoint,
          std::optional<std::string>& al_entrypoint,
          absl::flat_hash_map<std::string, bool>& static_flags) {
         if (j.contains(name)) {
            read_state_ps_entrypoint(name, entrypoint.emplace(), al_entrypoint,
                                     static_flags);
         }
         else {
            entrypoint = std::nullopt;
         }
      };

   read_state_entrypoint("vertex_shader"s, state.vs_entrypoint, state.vs_static_flags);
   read_state_ps_entrypoint("pixel_shader"s, state.ps_entrypoint,
                            state.ps_al_entrypoint, state.ps_static_flags);

   read_state_entrypoint_optional("pixel_oit_shader"s, state.ps_oit_entrypoint,
                                  state.ps_al_oit_entrypoint,
                                  state.ps_oit_static_flags);
}

void from_json(const nlohmann::json& j, Group_definition& definition)
{
   j.at("group_name"s).get_to(definition.group_name);
   j.at("source_name"s).get_to(definition.source_name);

   j.at("entrypoints"s).get_to(definition.entrypoints);

   get_to_if(j, "input_layouts"s, definition.input_layouts);
   get_to_if(j, "rendertypes"s, definition.rendertypes);
   get_to_if(j, "states"s, definition.states);
}

namespace {

auto load_group_definition(const std::filesystem::path& path) noexcept -> Group_definition
{
   try {
      Group_definition definition = nlohmann::json::parse(load_string_file(path));

      definition.last_write_time = std::filesystem::last_write_time(path);

      return definition;
   }
   catch (std::exception& e) {
      if (retry_dialog("Shader Definition Error"s,
                       "Failed to load shader group definition!\nfile: {}\nreason: {}"sv,
                       path.string(), e.what())) {
         return load_group_definition(path);
      }

      log_and_terminate("Failed to load shader group definition '"sv,
                        path.filename().string(), "' reason: "sv, e.what());
   }
}

}

auto load_group_definitions(const std::filesystem::path& source_path) noexcept
   -> std::vector<Group_definition>
{

   using namespace std::views;

   std::vector<Group_definition> definitions;

   for (std::filesystem::recursive_directory_iterator directory_iter{source_path};
        auto definition :
        directory_iter |
           filter([](const auto& entry) { return entry.is_regular_file(); }) |
           transform([](const auto& entry) { return entry.path(); }) |
           filter([](const auto& path) { return path.extension() == L".json"sv; }) |
           transform(load_group_definition)) {
      definitions.emplace_back(std::move(definition));
   }

   return definitions;
}

}
