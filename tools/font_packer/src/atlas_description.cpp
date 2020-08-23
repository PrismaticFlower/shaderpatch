
#include "atlas_description.hpp"

#include <fstream>

#include <nlohmann/json.hpp>

using namespace std::literals;

namespace sp {

auto read_atlas_description(const std::filesystem::path& path)
   -> std::unordered_map<char16_t, Atlas_location>
{
   std::ifstream file{path};

   nlohmann::json desc;
   file >> desc;

   std::unordered_map<char16_t, Atlas_location> glyphs;

   const float width = desc["atlas"s]["width"s] - 0.5f;
   const float height = desc["atlas"s]["height"s] - 0.5f;

   for (auto& glyph : desc["glyphs"s]) {
      auto& bounds = glyph["atlasBounds"s];

      if (bounds.empty()) continue;

      glyphs[static_cast<char16_t>(glyph["unicode"s])] =
         {.left = bounds["left"s] / width,
          .right = bounds["right"s] / width,
          .top = (height - bounds["top"s]) / height,
          .bottom = (height - bounds["bottom"s]) / height};
   }

   return glyphs;
}

}