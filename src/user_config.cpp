
#include "user_config.hpp"

#include <filesystem>

namespace sp {

using namespace std::literals;

User_config user_config = User_config{R"(shader patch.yml)"s};

User_config::User_config(const std::string& path) noexcept
{
   using namespace std::literals;

   try {
      parse_file(path);
   }
   catch (std::exception& e) {
      log(Log_level::warning, "Failed to read config file "sv,
          std::quoted(path), ". reason:"sv, e.what());
   }
}

void User_config::parse_file(const std::string& path)
{
   using namespace std::literals;

   const auto config = YAML::LoadFile(path);

   display.screen_percent =
      std::clamp(config["Display"s]["Screen Percent"s].as<std::uint32_t>(), 10u, 100u);

   display.allow_tearing = config["Display"s]["Allow Tearing"s].as<bool>();

   display.centred = config["Display"s]["Centred"s].as<bool>();

   graphics.antialiasing_method = aa_method_from_string(
      config["Graphics"s]["Anti-Aliasing Method"s].as<std::string>());

   graphics.antialiasing_sample_count = to_sample_count(graphics.antialiasing_method);

   effects.enabled = config["Effects"s]["Enabled"s].as<bool>();

   effects.color_quality = color_quality_from_string(
      config["Effects"s]["Color Quality"].as<std::string>());

   effects.bloom = config["Effects"s]["Bloom"s].as<bool>();

   effects.vignette = config["Effects"s]["Vignette"s].as<bool>();

   effects.film_grain = config["Effects"s]["Film Grain"s].as<bool>();

   effects.colored_film_grain =
      config["Effects"s]["Allow Colored Film Grain"s].as<bool>();

   developer.toggle_key = config["Developer"s]["Screen Toggle"s].as<int>();

   developer.unlock_fps = config["Developer"s]["Unlock FPS"s].as<bool>();

   developer.allow_event_queries =
      config["Developer"s]["Allow Event Queries"s].as<bool>();

   developer.force_d3d11_debug_layer =
      config["Developer"s]["Force D3D11 Debug Layer"s].as<bool>();
}
}
