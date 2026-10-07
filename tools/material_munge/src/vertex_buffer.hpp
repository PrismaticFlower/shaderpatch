#pragma once

#include "enum_flags.hpp"
#include "ucfb_editor.hpp"
#include "ucfb_reader.hpp"

#include <array>
#include <cstddef>
#include <limits>
#include <memory>

#include <glm/glm.hpp>

namespace sp {

enum class Vbuf_flags : std::uint32_t {
   position = 0b10u,
   blendindices = 0b100u,
   blendweight = 0b1000u,
   normal = 0b100000u,
   tangents = 0b1000000u,
   color = 0b10000000u,
   static_lighting = 0b100000000u,
   texcoords = 0b1000000000u,

   position_compressed = 0b1000000000000u,
   blendinfo_compressed = 0b10000000000000u,
   normal_compressed = 0b100000000000000u,
   texcoord_compressed = 0b1000000000000000u
};

template<>
struct is_enum_flag<Vbuf_flags> : std::true_type {};

struct Vertex_buffer {
   Vertex_buffer() = default;

   Vertex_buffer(const std::size_t count, const Vbuf_flags flags) noexcept;

   std::size_t count;

   std::unique_ptr<glm::vec3[]> positions;
   std::unique_ptr<glm::uint32[]> blendindices;
   std::unique_ptr<glm::vec3[]> blendweights;
   std::unique_ptr<glm::vec3[]> normals;
   std::unique_ptr<glm::vec3[]> tangents;
   std::unique_ptr<float[]> bitangent_signs;
   std::unique_ptr<glm::vec3[]> binormals;
   std::unique_ptr<glm::uint32[]> colors;
   std::unique_ptr<glm::uint32[]> static_lighting_colors;
   std::unique_ptr<glm::vec2[]> texcoords;
};

auto create_vertex_buffer(ucfb::Reader_strict<"VBUF"_mn> vbuf,
                          const std::array<glm::vec3, 2> vert_box) -> Vertex_buffer;

void output_vertex_buffer(const Vertex_buffer& vertex_buffer,
                          ucfb::Editor_data_writer& writer,
                          const std::array<glm::vec3, 2> vert_box);

class Vertex_position_decompress {
public:
   Vertex_position_decompress(const std::array<glm::vec3, 2> vert_box) noexcept
   {
      _mul = (vert_box[1] - vert_box[0]) * 0.5f / 32767.0f;
      _add = (vert_box[1] + vert_box[0]) * 0.5f / 32767.0f;
   }

   glm::vec3 operator()(const glm::i16vec4 compressed) const noexcept
   {
      const auto c = static_cast<glm::vec3>(compressed);

      return c * _mul + _add;
   }

private:
   glm::vec3 _mul;
   glm::vec3 _add;
};

class Vertex_position_compress {
public:
   Vertex_position_compress(const std::array<glm::vec3, 2> vert_box) noexcept
   {
      _sub = (vert_box[1] + vert_box[0]) * 0.5f;
      _mul = glm::vec3{32767.0f, 32767.0f, 32767.0f} * 2.0f /
             (vert_box[1] - vert_box[0]);
   }

   glm::i16vec4 operator()(const glm::vec3 pos) const noexcept
   {
      return {static_cast<glm::i16vec3>(glm::floor((pos - _sub) * _mul + 0.5f)), 0};
   }

private:
   glm::vec3 _sub;
   glm::vec3 _mul;
};
}
