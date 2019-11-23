#pragma once

#include <nlohmann/json.hpp>

namespace sp {

struct Material_options {
   bool transparent = false;
   bool hard_edged = false;
   bool double_sided = false;
   bool unlit = false;
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
}

}
