
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

class Terrain_indexer {
public:
   Terrain_indexer(const Terr_header& header)
   {
      _terrain_length = header.terrain_length;
      _out_length = std::abs(header.extents[0] - header.extents[2]) + 1;
      _in_base = header.terrain_length / 2;
      _in_mask = header.terrain_length - 1;
      _out_offset_y = header.extents[3];
      _out_offset_x = header.extents[2];
   };

   auto in(const std::ptrdiff_t x, const std::ptrdiff_t y) const noexcept -> std::ptrdiff_t
   {
      return (((_in_base + y) & _in_mask) * _terrain_length) +
             ((_in_base + x) & _in_mask);
   }

   auto out(const std::ptrdiff_t x, const std::ptrdiff_t y) const noexcept -> std::ptrdiff_t
   {
      return ((y + _out_offset_y) * _out_length) + (x + _out_offset_x);
   }

private:
   int _terrain_length = 0;
   int _in_base = 0;
   int _in_mask = 0;
   int _out_length = 0;
   int _out_offset_y = 0;
   int _out_offset_x = 0;
};

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

void read_height_map(std::ifstream& file, const glm::vec3 terrain_offset,
                     const Terr_header& header, const Terrain_indexer& indexer,
                     const std::span<glm::vec3> output)
{
   std::vector<std::int16_t> fixed_point_heightmap;
   fixed_point_heightmap.resize(header.terrain_length * header.terrain_length);

   file.read(reinterpret_cast<char*>(fixed_point_heightmap.data()),
             fixed_point_heightmap.size() * sizeof(std::int16_t));

   for (std::ptrdiff_t y = header.extents[1]; y <= header.extents[3]; ++y) {
      for (std::ptrdiff_t x = header.extents[0]; x <= header.extents[2]; ++x) {
         output[indexer.out(x, y)] =
            glm::vec3{x * header.grid_scale,
                      fixed_point_heightmap[indexer.in(x, y)] * header.height_scale,
                      -(y * header.grid_scale)} +
            terrain_offset;
      }
   }
}

void read_color_maps(std::ifstream& file, const Terr_header& header,
                     const Terrain_indexer& indexer, const std::span<glm::vec3> output)
{
   std::vector<Terrain_color> unorm_foreground;
   std::vector<Terrain_color> unorm_background;
   unorm_foreground.resize(header.terrain_length * header.terrain_length);
   unorm_background.resize(header.terrain_length * header.terrain_length);

   file.read(reinterpret_cast<char*>(unorm_foreground.data()),
             unorm_foreground.size() * sizeof(Terrain_color));
   file.read(reinterpret_cast<char*>(unorm_background.data()),
             unorm_background.size() * sizeof(Terrain_color));

   for (std::ptrdiff_t y = header.extents[1]; y <= header.extents[3]; ++y) {
      for (std::ptrdiff_t x = header.extents[0]; x <= header.extents[2]; ++x) {
         const glm::vec4 foreground = unorm_foreground[indexer.in(x, y)];
         const glm::vec4 background = unorm_foreground[indexer.in(x, y)];
         const glm::vec3 background_premult = background.rgb * background.a;

         output[indexer.out(x, y)] = (background_premult * (1.f - foreground.a)) +
                                     (foreground.rgb * foreground.a);
      }
   }
}

void read_diffuse_lighting(std::ifstream& file, const Terr_header& header,
                           const Terrain_indexer& indexer,
                           const std::span<glm::vec3> output)
{
   std::vector<Terrain_color> unorm_lighting;
   unorm_lighting.resize(header.terrain_length * header.terrain_length);

   file.read(reinterpret_cast<char*>(unorm_lighting.data()),
             unorm_lighting.size() * sizeof(Terrain_color));

   for (std::ptrdiff_t y = header.extents[1]; y <= header.extents[3]; ++y) {
      for (std::ptrdiff_t x = header.extents[0]; x <= header.extents[2]; ++x) {
         output[indexer.out(x, y)] = glm::vec4{unorm_lighting[indexer.in(x, y)]}.xyz;
      }
   }
}

void read_texture_weights(std::ifstream& file,
                          const std::array<std::uint8_t, 16>& texture_remap,
                          const Terr_header& header, const Terrain_indexer& indexer,
                          const std::span<std::array<float, 16>> output)
{
   std::vector<std::array<std::uint8_t, 16>> unorm_weights;
   unorm_weights.resize(header.terrain_length * header.terrain_length);

   file.read(reinterpret_cast<char*>(unorm_weights.data()),
             unorm_weights.size() * sizeof(std::array<std::uint8_t, 16>));

   for (std::ptrdiff_t y = header.extents[1]; y <= header.extents[3]; ++y) {
      for (std::ptrdiff_t x = header.extents[0]; x <= header.extents[2]; ++x) {
         auto& unorm_weight = unorm_weights[indexer.in(x, y)];

         for (auto i = 0; i < unorm_weight.size(); ++i) {
            output[indexer.out(x, y)][texture_remap[i]] = unorm_weight[i] / 255.f;
         }
      }
   }
}

auto read_terrain_cuts(std::ifstream& file) -> std::vector<Terrain_cut>
{
   std::int32_t size{};

   file.read(reinterpret_cast<char*>(&size), sizeof(size));

   if (size < 4) return {};

   std::int32_t count{};

   file.read(reinterpret_cast<char*>(&count), sizeof(count));

   std::vector<Terrain_cut> cuts;
   cuts.reserve(count);

   for (auto i = 0; i < count; ++i) {
      auto& cut = cuts.emplace_back();

      std::int32_t plane_count{};
      file.read(reinterpret_cast<char*>(&plane_count), sizeof(plane_count));

      std::array<glm::vec3, 2> aabb;
      file.read(reinterpret_cast<char*>(aabb.data()), sizeof(aabb));

      cut.centre = aabb[0] + aabb[1] / 2.0f;
      cut.radius = glm::distance(aabb[0], aabb[1]) / 2.0f;

      cut.planes.resize(plane_count);

      for (auto& plane : cut.planes) {
         file.read(reinterpret_cast<char*>(&plane), sizeof(aabb));
      }
   }

   return cuts;
}
}

Terrain_map::Terrain_map(const std::uint16_t length)
   : length{length},
     position{std::make_unique<glm::vec3[]>(length * length)},
     color{std::make_unique<glm::vec3[]>(length * length)},
     diffuse_lighting{std::make_unique<glm::vec3[]>(length * length)},
     texture_weights{std::make_unique<std::array<float, 16>[]>(length * length)}
{
}

Terrain_map::Terrain_map(const Terrain_map& other)
   : length{other.length},
     texture_names{other.texture_names},
     detail_texture{other.detail_texture},
     cuts{other.cuts},
     position{std::make_unique<glm::vec3[]>(length * length)},
     color{std::make_unique<glm::vec3[]>(length * length)},
     diffuse_lighting{std::make_unique<glm::vec3[]>(length * length)},
     texture_weights{std::make_unique<std::array<float, 16>[]>(length * length)}
{
   std::copy_n(other.position.get(), length * length, this->position.get());
   std::copy_n(other.color.get(), length * length, this->color.get());
   std::copy_n(other.diffuse_lighting.get(), length * length,
               this->diffuse_lighting.get());
   std::copy_n(other.texture_weights.get(), length * length,
               this->texture_weights.get());
}

auto load_terrain_map(const std::filesystem::path& path,
                      const glm::vec3 terrain_offset) -> Terrain_map
{
   std::ifstream file{path, std::ios::binary};
   file.exceptions(std::ios::badbit | std::ios::failbit);

   Terr_header header;

   file.read(reinterpret_cast<char*>(&header), sizeof(header));

   const Terrain_indexer indexer{header};

   Terrain_map map{static_cast<std::uint16_t>(
      std::abs(header.extents[0] - header.extents[2]) + 1)};
   std::array<std::uint8_t, 16> texture_remap;

   std::tie(map.texture_names, texture_remap) = read_texture_names(header);
   map.texture_transforms = read_texture_transforms(header, texture_remap);
   map.detail_texture = header.texture_names[0].detail;

   // Skip Decal Tiles
   file.seekg(sizeof(Decal_tile) * header.decal_settings.tile_count + 8, std::ios::cur);

   read_height_map(file, terrain_offset, header, indexer,
                   std::span{map.position.get(),
                             static_cast<std::size_t>(map.length * map.length)});
   read_color_maps(file, header, indexer,
                   std::span{map.color.get(),
                             static_cast<std::size_t>(map.length * map.length)});

   if (header.prelit) {
      read_diffuse_lighting(file, header, indexer,
                            std::span{map.diffuse_lighting.get(),
                                      static_cast<std::size_t>(map.length * map.length)});
   }

   read_texture_weights(file, texture_remap, header, indexer,
                        std::span{map.texture_weights.get(),
                                  static_cast<std::size_t>(map.length * map.length)});

   try {
      const auto terrain_length_half_sq =
         (header.terrain_length / 2) * (header.terrain_length / 2);

      file.seekg(terrain_length_half_sq / 2, std::ios::cur); // Unknown data
      file.seekg(terrain_length_half_sq / 2, std::ios::cur); // Unknown data
      file.seekg((terrain_length_half_sq / 4) * 3,
                 std::ios::cur);         // Patch data
      file.seekg(131072, std::ios::cur); // Foliage map
      file.seekg(262144, std::ios::cur); // Unknown data
      file.seekg(131072, std::ios::cur); // Unknown data

      map.cuts = read_terrain_cuts(file);
   }
   catch (std::ios_base::failure&) {
      // Sometimes terrain files end abruptly, terrainmunge
      // and Zero Editor still treat them as valid however
      // so we must be prepared for it to happen here.
   }

   return map;
}
}
