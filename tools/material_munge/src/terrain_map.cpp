
#include "terrain_map.hpp"
#include "enum_flags.hpp"
#include "srgb_conversion.hpp"
#include "string_utilities.hpp"

#include <algorithm>
#include <cmath>
#include <execution>
#include <fstream>
#include <string_view>
#include <tuple>

namespace sp {

namespace {
enum class Munge_flags : std::uint8_t {
   terrain = 0b1,
   water = 0b10,
   foliage = 0b100,
   extra_light_map = 0b1000
};

constexpr bool marked_as_enum_flag(Munge_flags)
{
   return true;
};

struct Terrain_string {
   char chars[32];

   constexpr operator std::string_view() const noexcept
   {
      std::string_view sv{chars, sizeof(chars)};

      if (const auto offset = sv.find_first_of('\0'); offset != sv.npos)
         sv.remove_suffix(sv.size() - offset);

      return sv;
   }
};

static_assert(sizeof(Terrain_string) == 32);

struct Terrain_color {
   std::uint8_t blue;
   std::uint8_t green;
   std::uint8_t red;
   std::uint8_t alpha;

   operator glm::vec4() const noexcept
   {
      return decompress_srgb(
         glm::vec4{red / 255.f, green / 255.f, blue / 255.f, alpha / 255.f});
   };
};

static_assert(sizeof(Terrain_color) == sizeof(std::uint32_t));

struct Texture_name {
   Terrain_string diffuse;
   Terrain_string detail;
};

struct Water_settings {
   float height;
   float unused[3];
   float u_velocity;
   float v_velocity;
   float u_repeat;
   float v_repeat;
   Terrain_color colour;
   Terrain_string texture;
};

struct Decal_settings {
   struct {
      Terrain_string name;
   } textures[16];
   std::int32_t tile_count;
};

struct Decal_coords {
   float x;
   float z;
};

struct Decal_tile {
   std::int32_t x;
   std::int32_t z;
   std::int32_t texture;
   Decal_coords coords[4];
};

#pragma pack(push, 1) // Instead of using this you could manually read in and align the header after munge_flags

struct Terr_header {
   char mn[4];
   std::int32_t version; // 21 = SWBF1, 22 = SWBFII, this file assumes SWBFII
   std::int16_t extents[4];
   std::int32_t unknown;
   float tex_scales[16];
   Terrain_texture_axis tex_axes[16];
   float tex_rotations[16];
   float height_scale; // in meters
   float grid_scale;   // in meters
   std::int32_t prelit;
   std::int32_t terrain_length;
   std::int32_t grids_per_foliage; // unclear if any value other than 2 is valid here, Zero always saves out 2
   Munge_flags munge_flags; // Not present in SWBF1 terrain files.
   Texture_name texture_names[16];
   Water_settings water_settings_0; // unused
   Water_settings water_settings;
   Water_settings water_settings_2_14[14]; // unused
   Decal_settings decal_settings;          // unused
};

static_assert(sizeof(Terr_header) == 2813);

#pragma pack(pop)

auto read_texture_names(const Terr_header& header) noexcept
   -> std::tuple<std::vector<std::string>, std::array<std::uint8_t, 16>>
{
   std::tuple<std::vector<std::string>, std::array<std::uint8_t, 16>> result;
   auto& textures = std::get<0>(result);
   auto& texture_remap = std::get<1>(result);

   textures.reserve(16);

   for (int i = 0; i < 16; ++i) {
      const std::string_view str = header.texture_names[i].diffuse;

      if (str.empty()) {
         texture_remap[i] = 15;
         continue;
      }

      texture_remap[i] = static_cast<std::uint8_t>(textures.size());
      textures.emplace_back(str);
   }

   return result;
}

auto read_texture_transforms(const Terr_header& header,
                             const std::array<std::uint8_t, 16>& texture_remap)
   -> std::array<Terrain_texture_transform, 16>
{
   std::array<Terrain_texture_transform, 16> transforms;

   for (auto i = 0; i < transforms.size(); ++i) {
      auto& trans = transforms[texture_remap[i]];

      trans.set_axis(header.tex_axes[i]);
      trans.set_scale(1.0f / header.tex_scales[i]);
   }

   return transforms;
}

}

Terrain_map::Terrain_map(const std::uint16_t length) : length{length} {}

auto load_terrain_map(const std::filesystem::path& path) -> Terrain_map
{
   std::ifstream file{path, std::ios::binary};
   file.exceptions(std::ios::badbit | std::ios::failbit);

   Terr_header header;

   file.read(reinterpret_cast<char*>(&header), sizeof(header));

   Terrain_map map{static_cast<std::uint16_t>(header.terrain_length)};
   std::array<std::uint8_t, 16> texture_remap;

   map.grid_scale = header.grid_scale;
   map.world_size = header.terrain_length * header.grid_scale;

   std::tie(map.texture_names, texture_remap) = read_texture_names(header);
   map.texture_transforms = read_texture_transforms(header, texture_remap);
   map.detail_texture = header.texture_names[0].detail;

   // Skip Decal Tiles
   file.seekg(sizeof(Decal_tile) * header.decal_settings.tile_count + 8, std::ios::cur);

   // Skip height map
   file.seekg(sizeof(std::int16_t) * map.length * map.length, std::ios::cur);

   // Skip color maps
   file.seekg(sizeof(std::uint32_t) * map.length * map.length, std::ios::cur);
   file.seekg(sizeof(std::uint32_t) * map.length * map.length, std::ios::cur);

   if ((header.munge_flags & Munge_flags::extra_light_map) ==
       Munge_flags::extra_light_map) {
      file.seekg(sizeof(std::uint32_t) * map.length * map.length, std::ios::cur);
   }

   map.texture_weights.resize(map.length * map.length);

   file.read(reinterpret_cast<char*>(map.texture_weights.data()),
             map.length * map.length * sizeof(std::array<std::uint8_t, 16>));

   for (auto& weight : map.texture_weights) {
      for (std::size_t i = 0; i < 16; ++i) {
         weight[texture_remap[i]] = weight[i];
      }
   }

   return map;
}
}
