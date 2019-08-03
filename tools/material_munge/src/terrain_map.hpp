#pragma once

#include "terrain_cut.hpp"
#include "terrain_texture_transform.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <gsl/gsl>

namespace sp {

struct Terrain_map {
   explicit Terrain_map(const std::uint16_t length);

   Terrain_map(Terrain_map&&) = default;
   Terrain_map& operator=(Terrain_map&&) = default;

   Terrain_map(const Terrain_map&) = delete;
   Terrain_map& operator=(const Terrain_map&) = delete;

   ~Terrain_map() = default;

   std::uint16_t length{};
   std::vector<std::string> texture_names;
   std::array<Terrain_texture_transform, 16> texture_transforms;
   std::string detail_texture;
   std::vector<Terrain_cut> cuts;

   std::unique_ptr<glm::vec3[]> position;
   std::unique_ptr<glm::vec3[]> color;
   std::unique_ptr<glm::vec3[]> diffuse_lighting;
   std::unique_ptr<std::array<float, 16>[]> texture_weights;
};

auto load_terrain_map(const std::filesystem::path& path) -> Terrain_map;
}
