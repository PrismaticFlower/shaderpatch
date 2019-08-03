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

enum class Terrain_rendertype { normal_ext, pbr };

struct Terrain_material {
   std::string albedo_map;
   std::string normal_map;
   std::string metallic_roughness_map;
   std::string ao_map;
   std::string height_map;
   std::string diffuse_map;
   std::string gloss_map;
   float height_scale;
};

struct Terrain_materials_config {
   bool use_envmap = false;
   std::string envmap_name;

   Terrain_bumpmapping bumpmapping = Terrain_bumpmapping::parallax_offset_mapping;
   Terrain_rendertype rendertype = Terrain_rendertype::normal_ext;

   bool use_global_detail_map = false;
   std::string global_detail_map;

   glm::vec3 base_color;
   float base_metallicness;
   float base_roughness;

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

      const auto bumpmapping =
         global["BumpMappingType"s].as<std::string>("Parallax Offset Mapping"s);

      if (bumpmapping == "Normal Mapping"sv) {
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

      const auto rendertype = global["Rendertype"s].as<std::string>("normal_ext"s);

      if (rendertype == "normal_ext"sv) {
         config.rendertype = sp::Terrain_rendertype::normal_ext;
      }
      else if (rendertype == "pbr"sv) {
         config.rendertype = sp::Terrain_rendertype::pbr;
      }
      else {
         throw std::runtime_error{"Invalid Rendertype"s};
      }

      config.use_global_detail_map = global["UseGlobalDetailMap"s].as<bool>(false);
      config.global_detail_map =
         global["GlobalDetailMap"s].as<std::string>("$null_detailmap"s);

      config.base_color =
         global["BaseColor"s].as<glm::vec3>(glm::vec3{1.f, 1.f, 1.f});
      config.base_metallicness = global["BaseMetallicness"s].as<float>(1.f);
      config.base_roughness = global["BaseRoughness"s].as<float>(1.f);

      for (auto& entry : node["Materials"s]) {
         config.materials.emplace(entry.first.as<std::string>(),
                                  entry.second.as<sp::Terrain_material>());
      }

      return true;
   }
};
}
