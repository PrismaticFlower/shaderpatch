#pragma once

#include "ucfb_reader.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4127)
#pragma warning(disable : 4251)
#pragma warning(disable : 4275)

#include <yaml-cpp/yaml.h>
#pragma warning(pop)

namespace sp {

enum class Color_grading_region_shape : std::uint32_t { box, sphere, cylinder };

struct Color_grading_region_desc {
   std::string name;
   std::string config_name;
   Color_grading_region_shape shape = Color_grading_region_shape::box;
   glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
   glm::vec3 position = {0.0f, 0.0f, 0.0f};
   glm::vec3 size = {0.0f, 0.0f, 0.0f};
   float fade_length = 1.0f;
};

struct Color_grading_regions {
   std::vector<Color_grading_region_desc> regions;
   std::unordered_map<std::string, YAML::Node> configs;
};

void write_colorgrading_regions(const std::filesystem::path& save_path,
                                const Color_grading_regions& colorgrading_regions);

auto load_colorgrading_regions(ucfb::Reader_strict<"clrg"_mn> reader)
   -> Color_grading_regions;

}