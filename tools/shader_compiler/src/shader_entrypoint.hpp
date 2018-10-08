#pragma once

#include "compose_exception.hpp"
#include "preprocessor_defines.hpp"
#include "shader_custom_flags.hpp"

#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

namespace sp::shader {

enum class Stage { vertex, pixel };

struct Vertex_state {
   struct Generic_input {
      bool dynamic_compression = false;
      bool position = false;
      bool skinned = false;
      bool normal = false;
      bool tangents = false;
      bool color = false;
      bool texture_coords = false;
   };

   Generic_input generic_input;
};

struct Pixel_state {
   bool lighting = false;
};

struct Entrypoint {
   Stage stage;
   std::optional<std::string> function_name;

   std::variant<Vertex_state, Pixel_state> stage_state{};

   Custom_flags static_flags;

   Preprocessor_defines defines;
};

inline void from_json(const nlohmann::json& j, Stage& stage)
{
   using namespace std::literals;

   if (const auto string = j.get<std::string>(); string == "vertex"s) {
      stage = Stage::vertex;
   }
   else if (string == "pixel"s) {
      stage = Stage::pixel;
   }
   else {
      throw compose_exception<std::invalid_argument>('"', string, '"',
                                                     " is not a valid shader stage"s);
   }
}

inline void from_json(const nlohmann::json& j, Vertex_state& vertex_state)
{
   using namespace std::literals;

   vertex_state.generic_input =
      j.value("generic_input"s, Vertex_state::Generic_input{});
}

inline void from_json(const nlohmann::json& j, Vertex_state::Generic_input& generic_input)
{
   using namespace std::literals;

   generic_input.dynamic_compression = j.at("dynamic_compression"s);
   generic_input.position = j.at("position"s);
   generic_input.skinned = j.at("skinned"s);
   generic_input.normal = j.at("normal"s);
   generic_input.tangents = j.at("tangents"s);
   generic_input.color = j.at("color"s);
   generic_input.texture_coords = j.at("texture_coords"s);
}

inline void from_json(const nlohmann::json& j, Pixel_state& pixel_state)
{
   using namespace std::literals;

   pixel_state.lighting = j.at("lighting"s);
}

inline void from_json(const nlohmann::json& j, Entrypoint& entrypoint)
{
   using namespace std::literals;

   entrypoint.stage = j.at("stage"s);

   if (j.count("function_name"s)) {
      entrypoint.function_name = j.at("function_name"s).get<std::string>();
   }

   switch (entrypoint.stage) {
   case Stage::vertex:
      entrypoint.stage_state = j.at("vertex_state"s).get<Vertex_state>();
      break;
   case Stage::pixel:
      entrypoint.stage_state = j.at("pixel_state"s).get<Pixel_state>();
      break;
   }

   entrypoint.static_flags = j.value("static_flags"s, Custom_flags{});

   entrypoint.defines.move_in_defines(
      j.value("defines"s, std::vector<Shader_define>{}));
   entrypoint.defines.move_in_undefines(
      j.value("undefines"s, std::vector<std::string>{}));
}

}
