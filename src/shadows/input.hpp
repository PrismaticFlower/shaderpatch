#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace sp::shadows {

struct Input_model {
   enum class Topology { tri_list, tri_strip };

   struct Segment {
      Topology topology = Topology::tri_strip;
      std::uint32_t vertex_count = 0;
      std::uint32_t primitive_count = 0;

      bool doublesided = false;
      bool hardedged = false;

      std::string diffuse_texture;

      std::vector<std::uint16_t> indices;
      std::vector<std::array<std::int16_t, 3>> vertices;

      /// @brief May be empty if hardedged is false.
      std::vector<std::array<std::int16_t, 2>> texcoords;
   };

   std::string name;

   glm::vec3 min_vertex;
   glm::vec3 max_vertex;

   std::vector<Segment> segments;
};

struct Input_game_model {
   std::string name;

   std::string lod0;
   std::uint32_t lod0_tris = 0;

   std::string lod1;
   std::uint32_t lod1_tris = 0;

   std::string lod2;
   std::uint32_t lod2_tris = 0;

   std::string lowd;
   std::uint32_t lowd_tris = 0;
};

struct Input_entity_class {
   std::string base_name;
   std::string type_name;
   std::string geometry_name;
};

struct Input_object_instance {
   std::string name;
   std::string type_name;

   /// @brief Name of the layer this instance is from. Provided as a string view to avoid needlessly copying it for each instance.
   std::string_view layer_name;

   /// @brief If not empty names the model to use instead of the one referenced in the entity class.
   std::string geometry_name_override;

   std::array<glm::vec3, 3> rotation;
   glm::vec3 positionWS;

   bool in_child_lvl = false;
};

struct Input_animation_group {
   std::string name;

   std::vector<std::string> objects;
};

}