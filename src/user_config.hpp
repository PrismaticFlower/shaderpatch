#pragma once

#include "logger.hpp"

#include <cstdint>
#include <string>

#include <INIReader.h>
#include <glm/glm.hpp>

namespace sp {

struct User_config {
   User_config() = default;

   explicit User_config(std::string path) noexcept
   {
      using namespace std::literals;

      INIReader reader{path};

      if (reader.ParseError() < 0) {
         log(Log_level::warning, "Failed to parse config file \'",
             "shader patch.ini"s, '\'');
      }

      display.enabled = reader.GetBoolean("display"s, "Enabled"s, false);
      display.windowed = reader.GetBoolean("display"s, "Windowed"s, false);
      display.borderless = reader.GetBoolean("display"s, "Borderless"s, true);
      display.resolution.x = reader.GetInteger("display"s, "Width"s, 800);
      display.resolution.y = reader.GetInteger("display"s, "Height"s, 600);

      rendering.high_res_reflections =
         reader.GetBoolean("rendering", "HighResolutionReflections"s, true);

      auto reflection_scale =
         reader.GetReal("rendering", "ReflectionBufferScale"s, 1.0);

      rendering.reflection_buffer_factor =
         static_cast<int>(1.0 / glm::clamp(reflection_scale, 0.01, 1.0));

      auto refraction_scale =
         reader.GetReal("rendering", "RefractionBufferScale"s, 0.5);

      rendering.refraction_buffer_factor =
         static_cast<int>(1.0 / glm::clamp(refraction_scale, 0.01, 1.0));

      debugscreen.activate_key =
         reader.GetInteger("debugscreen"s, "ActivateVirtualKey"s, 0);
   }

   struct {
      bool enabled = false;
      bool windowed = false;
      glm::ivec2 resolution{800, 600};

      bool borderless = true;
   } display;

   struct {
      bool high_res_reflections = true;

      int reflection_buffer_factor = 1;
      int refraction_buffer_factor = 2;
   } rendering;

   struct {
      std::uintptr_t activate_key{0};
   } debugscreen;
};
}
