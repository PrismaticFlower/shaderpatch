#pragma once

#include <nlohmann/json.hpp>

#include <optional>

namespace sp {

struct Material_options {
   bool transparent = false;
   bool hard_edged = false;
   bool double_sided = false;
   bool unlit = false;
   std::optional<bool> forced_transparent_value = std::nullopt;
   std::optional<bool> forced_hard_edged_value = std::nullopt;
   std::optional<bool> forced_double_sided_value = std::nullopt;
   std::optional<bool> forced_unlit_value = std::nullopt;
   bool compressed = true;
   bool generate_tangents = true;
};

inline void to_json(nlohmann::json& j, const Material_options& options)
{
   j = nlohmann::json{{"transparent", options.transparent},
                      {"hard_edged", options.hard_edged},
                      {"double_sided", options.double_sided},
                      {"unlit", options.unlit},
                      {"compressed", options.compressed},
                      {"generate_tangents", options.generate_tangents}};

   if (options.forced_transparent_value) {
      j["forced_transparent_value"] = options.forced_transparent_value.value();
   }
   if (options.forced_hard_edged_value) {
      j["forced_hard_edged_value"] = options.forced_hard_edged_value.value();
   }
   if (options.forced_double_sided_value) {
      j["forced_double_sided_value"] = options.forced_double_sided_value.value();
   }
   if (options.forced_unlit_value) {
      j["forced_unlit_value"] = options.forced_unlit_value.value();
   }
}

inline void from_json(const nlohmann::json& j, Material_options& options)
{
   using namespace std::literals;

   options.transparent = j.at("transparent"s).get<bool>();
   options.hard_edged = j.at("hard_edged"s).get<bool>();
   options.double_sided = j.at("double_sided"s).get<bool>();
   options.unlit = j.at("unlit"s).get<bool>();
   options.compressed = j.at("compressed"s).get<bool>();
   options.generate_tangents = j.at("generate_tangents"s).get<bool>();

   if (j.contains("forced_transparent_value"s)) {
      options.forced_transparent_value =
         j.at("forced_transparent_value"s).get<bool>();
   }
   if (j.contains("forced_hard_edged_value"s)) {
      options.forced_hard_edged_value = j.at("forced_hard_edged_value"s).get<bool>();
   }
   if (j.contains("forced_double_sided_value"s)) {
      options.forced_double_sided_value =
         j.at("forced_double_sided_value"s).get<bool>();
   }
   if (j.contains("forced_unlit_value"s)) {
      options.forced_unlit_value = j.at("forced_unlit_value"s).get<bool>();
   }
}

}
