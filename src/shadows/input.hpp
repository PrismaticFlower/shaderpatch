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
   std::string lod1;
   std::string lod2;
   std::string lowd;
};

struct Input_object_class {
   std::string name;
   std::string game_model_name;
};

struct Input_instance {
   std::string name;
   std::string class_name;

   std::uint32_t layer_hash = 0;

   /// @brief If not empty names the model to use instead of the one referenced in class name.
   std::string model_override;

   std::array<glm::vec3, 3> rotation;
   glm::vec3 positionWS;
};

struct Input_animation_group {
   std::string name;

   std::vector<std::string> objects;
};

}