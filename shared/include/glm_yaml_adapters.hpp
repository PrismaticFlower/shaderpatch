#pragma once

#include <glm/glm.hpp>

#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4127)

#include <yaml-cpp/yaml.h>
#pragma warning(pop)

namespace YAML {

template<typename Vec>
struct convert_vec {
   static Node encode(const Vec& vec)
   {
      using namespace std::literals;

      YAML::Node node;

      for (typename Vec::length_type i = 0; i < vec.length(); ++i) {
         node.push_back(vec[i]);
      }

      return node;
   }

   static bool decode(const Node& node, Vec& vec)
   {
      if (!node.IsSequence() ||
          static_cast<typename Vec::length_type>(node.size()) != vec.length()) {
         return false;
      }

      for (typename Vec::length_type i = 0; i < vec.length(); ++i) {
         vec[i] = node[i].as<Vec::value_type>();
      }

      return true;
   }
};

template<>
struct convert<glm::vec2> : public convert_vec<glm::vec2> {
};

template<>
struct convert<glm::vec3> : public convert_vec<glm::vec3> {
};

template<>
struct convert<glm::vec4> : public convert_vec<glm::vec4> {
};

template<>
struct convert<glm::ivec2> : public convert_vec<glm::ivec2> {
};

template<>
struct convert<glm::ivec3> : public convert_vec<glm::ivec3> {
};

template<>
struct convert<glm::ivec4> : public convert_vec<glm::ivec4> {
};

template<>
struct convert<glm::uvec2> : public convert_vec<glm::uvec2> {
};

template<>
struct convert<glm::uvec3> : public convert_vec<glm::uvec3> {
};

template<>
struct convert<glm::uvec4> : public convert_vec<glm::uvec4> {
};
template<>
struct convert<glm::bvec2> : public convert_vec<glm::bvec2> {
};

template<>
struct convert<glm::bvec3> : public convert_vec<glm::bvec3> {
};

template<>
struct convert<glm::bvec4> : public convert_vec<glm::bvec4> {
};

}
