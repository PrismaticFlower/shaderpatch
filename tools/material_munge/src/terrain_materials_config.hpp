#pragma once

#include "glm_yaml_adapters.hpp"

#include <map>
#include <string>

#include <yaml-cpp/yaml.h>

namespace sp {
enum class Terrain_bumpmapping {
   normal_mapping,
   parallax_offset_mapping,
   parallax_occlusion_mapping
};

enum class Terrain_blending { height, basic };

enum class Terrain_rendertype { normal_ext, pbr };

enum class Terrain_far { downsampled, fullres };

struct Terrain_material {
   std::string albedo_map;
   std::string normal_map;
   std::string metallic_roughness_map;
   std::string ao_map;
   std::string height_map;
   std::string diffuse_map;
   std::string gloss_map;
   float height_scale;
   float specular_exponent;
};

struct Terrain_materials_config {
   bool use_envmap = false;
   std::string envmap_name;

   bool use_ze_static_lighting = false;
   bool srgb_diffuse_maps = false;

   Terrain_bumpmapping bumpmapping = Terrain_bumpmapping::parallax_offset_mapping;
   Terrain_blending blending = Terrain_blending::height;
   Terrain_rendertype rendertype = Terrain_rendertype::normal_ext;
   Terrain_far far_terrain = Terrain_far::downsampled;

   glm::vec3 base_color = {1.0f, 1.0f, 1.0f};
   float base_metallicness = 1.0f;
   float base_roughness = 1.0f;

   glm::vec3 diffuse_color = {1.0f, 1.0f, 1.0f};
   glm::vec3 specular_color = {1.0f, 1.0f, 1.0f};

   std::map<std::string, Terrain_material, std::less<>> materials;
};
}

namespace YAML {

template<>
struct convert<sp::Terrain_material> {
   static bool decode(const Node& node, sp::Terrain_material& material)
   {
      using namespace std::literals;

      material.albedo_map = node["AlbedoMap"s].as<std::string>("$null_albedomap"s);
      material.normal_map = node["NormalMap"s].as<std::string>("$null_normalmap"s);
      material.metallic_roughness_map =
         node["MetallicRoughnessMap"s].as<std::string>("$null_metallic_roughnessmap"s);
      material.ao_map = node["AOMap"s].as<std::string>("$null_ao"s);
      material.height_map = node["HeightMap"s].as<std::string>("$null_heightmap"s);
      material.diffuse_map = node["DiffuseMap"s].as<std::string>("$null_diffusemap"s);
      material.gloss_map = node["GlossMap"s].as<std::string>("$null_glossmap"s);
      material.height_scale = node["HeightScale"s].as<float>(0.05f);
      material.specular_exponent = node["SpecularExponent"s].as<float>(64.0f);

      return true;
   }
};

template<>
struct convert<sp::Terrain_materials_config> {
   static bool decode(const Node& node, sp::Terrain_materials_config& config)
   {
      using namespace std::literals;

      auto global = node["Global"];

      config.use_envmap = global["UseEnvironmentMapping"s].as<bool>(false);
      config.envmap_name = global["EnvironmentMap"s].as<std::string>(""s);
      config.use_ze_static_lighting = global["UseZEStaticLighting"s].as<bool>(false);
      config.srgb_diffuse_maps = global["sRGBDiffuseMaps"s].as<bool>(true);

      if (const auto bumpmapping =
             global["BumpMappingType"s].as<std::string>("Parallax Offset Mapping"s);
          bumpmapping == "Normal Mapping"sv) {
         config.bumpmapping = sp::Terrain_bumpmapping::normal_mapping;
      }
      else if (bumpmapping == "Parallax Offset Mapping"sv) {
         config.bumpmapping = sp::Terrain_bumpmapping::parallax_offset_mapping;
      }
      else if (bumpmapping == "Parallax Occlusion Mapping"sv) {
         config.bumpmapping = sp::Terrain_bumpmapping::parallax_occlusion_mapping;
      }
      else {
         throw std::runtime_error{"Invalid BumpMappingType"s};
      }

      if (const auto rendertype = global["Rendertype"s].as<std::string>("normal_ext"s);
          rendertype == "normal_ext"sv) {
         config.rendertype = sp::Terrain_rendertype::normal_ext;
      }
      else if (rendertype == "pbr"sv) {
         config.rendertype = sp::Terrain_rendertype::pbr;
      }
      else {
         throw std::runtime_error{"Invalid Rendertype"s};
      }

      if (const auto blending = global["BlendingMode"s].as<std::string>("Height"s);
          blending == "Height"sv) {
         config.blending = sp::Terrain_blending::height;
      }
      else if (blending == "Basic"sv) {
         config.blending = sp::Terrain_blending::basic;
      }
      else {
         throw std::runtime_error{"Invalid BlendingMode"s};
      }

      if (const auto lowres = global["FarTerrain"s].as<std::string>("Downsampled"s);
          lowres == "Downsampled"sv) {
         config.far_terrain = sp::Terrain_far::downsampled;
      }
      else if (lowres == "Fullres"sv) {
         config.far_terrain = sp::Terrain_far::fullres;
      }
      else {
         throw std::runtime_error{"Invalid FarTerrain"s};
      }

      config.base_color =
         global["BaseColor"s].as<glm::vec3>(glm::vec3{1.f, 1.f, 1.f});
      config.base_metallicness = global["BaseMetallicness"s].as<float>(1.f);
      config.base_roughness = global["BaseRoughness"s].as<float>(1.f);

      config.diffuse_color =
         global["DiffuseColor"s].as<glm::vec3>(glm::vec3{1.f, 1.f, 1.f});
      config.specular_color =
         global["SpecularColor"s].as<glm::vec3>(glm::vec3{1.f, 1.f, 1.f});

      for (auto& entry : node["Materials"s]) {
         config.materials.emplace(entry.first.as<std::string>(),
                                  entry.second.as<sp::Terrain_material>());
      }

      return true;
   }
};
}
