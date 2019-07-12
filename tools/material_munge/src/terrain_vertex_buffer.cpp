#include "terrain_vertex_buffer.hpp"
#include "vertex_buffer.hpp"

#include <stdexcept>

#include <glm/gtc/color_space.hpp>

namespace sp {

namespace {
struct Compressed_input_terrain_vertex {
   glm::i16vec3 position;
   glm::int16 blend_weight_1;

   glm::u8vec3 normal;
   glm::uint8 blend_weight_2;

   glm::uint32 static_lighting;
};

static_assert(sizeof(Compressed_input_terrain_vertex) == 16);

auto decompress_vertex(const Compressed_input_terrain_vertex compressed,
                       const std::array<std::uint8_t, 3> texture_indices,
                       const Vertex_position_decompress pos_decompress) noexcept
   -> Terrain_vertex
{
   Terrain_vertex vertex;

   vertex.position = pos_decompress(glm::i16vec4{compressed.position, 0});
   vertex.normal =
      glm::vec3{compressed.normal.z, compressed.normal.y, compressed.normal.x} /
         255.0f * 2.0f -
      1.0f;
   vertex.static_lighting = compressed.static_lighting;
   vertex.blend_weights =
      glm::vec2{compressed.blend_weight_1, compressed.blend_weight_2} / 255.0f;
   vertex.texture_indices = texture_indices;

   return vertex;
}

struct Packed_output_terrain_vertex {
   glm::i16vec3 position;
   glm::uint16 texture_indices_tbn_signs;
   glm::uint32 normal;
   glm::uint32 tangent;
};

static_assert(sizeof(Packed_output_terrain_vertex) == 16);

constexpr auto packed_vertex_flags =
   Vbuf_flags::position | Vbuf_flags::normal | Vbuf_flags::tangents |
   Vbuf_flags::texcoords | Vbuf_flags::position_compressed |
   Vbuf_flags::normal_compressed | Vbuf_flags::texcoord_compressed;

auto get_shadowing(const glm::uint static_lighting) -> float
{
   auto unpacked = glm::unpackUnorm4x8(static_lighting);

   unpacked = glm::convertSRGBToLinear(unpacked);

   const float shadowing =
      glm::dot(glm::vec3{unpacked}, glm::vec3{0.2126f, 0.7152f, 0.0722f});

   return glm::convertLinearToSRGB(glm::vec1{shadowing}).x;
}

auto pack_vertex(const Terrain_vertex& vertex,
                 const Vertex_position_compress& pos_compress) noexcept
   -> Packed_output_terrain_vertex
{
   Packed_output_terrain_vertex packed{};

   packed.position = pos_compress(vertex.position);

   packed.texture_indices_tbn_signs |= (vertex.normal.z >= 0.0f);
   packed.texture_indices_tbn_signs |= (vertex.tangent.z >= 0.0f) << 1;
   packed.texture_indices_tbn_signs |= (vertex.bitangent_sign >= 0.0f) << 2;
   packed.texture_indices_tbn_signs |= (vertex.texture_indices[0] & 0xf) << 3;
   packed.texture_indices_tbn_signs |= (vertex.texture_indices[1] & 0xf) << 7;
   packed.texture_indices_tbn_signs |= (vertex.texture_indices[2] & 0xf) << 11;

   packed.normal |=
      static_cast<glm::uint>(glm::clamp(vertex.normal.x * 0.5f + 0.5f, 0.0f, 1.0f) * 255.0f)
      << 16;
   packed.normal |=
      static_cast<glm::uint>(glm::clamp(vertex.normal.y * 0.5f + 0.5f, 0.0f, 1.0f) * 255.0f)
      << 8;
   packed.normal |= static_cast<glm::uint>(
      glm::clamp(vertex.tangent.x * 0.5f + 0.5f, 0.0f, 1.0f) * 255.0f);
   packed.normal |=
      static_cast<glm::uint>(glm::clamp(vertex.tangent.y * 0.5f + 0.5f, 0.0f, 1.0f) * 255.0f)
      << 24;

   packed.tangent |=
      static_cast<glm::uint>(glm::clamp(vertex.blend_weights[0], 0.0f, 1.0f) * 255.0f)
      << 16;
   packed.tangent |=
      static_cast<glm::uint>(glm::clamp(vertex.blend_weights[1], 0.0f, 1.0f) * 255.0f)
      << 8;
   packed.tangent |= static_cast<glm::uint>(
      glm::clamp(get_shadowing(vertex.static_lighting), 0.0f, 1.0f) * 255.0f);

   return packed;
}

}

auto create_terrain_vertex_buffer(ucfb::Reader_strict<"VBUF"_mn> vbuf,
                                  const std::array<std::uint8_t, 3> texture_indices,
                                  const std::array<glm::vec3, 2> vert_box) -> Terrain_vertex_buffer
{
   auto [count, stride, flags] =
      vbuf.read_multi<std::uint32_t, std::uint32_t, Vbuf_flags>();

   constexpr auto expected_flags =
      Vbuf_flags::position | Vbuf_flags::normal | Vbuf_flags::static_lighting |
      Vbuf_flags::position_compressed | Vbuf_flags::normal_compressed;

   if (flags != expected_flags) {
      throw std::runtime_error{"Terrain VBUF has unexpected vertex flags!"};
   }

   Terrain_vertex_buffer result;
   result.reserve(count);

   const Vertex_position_decompress pos_decompress{vert_box};

   for (auto vert : vbuf.read_array<Compressed_input_terrain_vertex>(count)) {
      result.emplace_back(decompress_vertex(vert, texture_indices, pos_decompress));
   }

   return result;
}

void output_vertex_buffer(const Terrain_vertex_buffer& vertex_buffer,
                          ucfb::Editor_data_writer& writer,
                          const std::array<glm::vec3, 2> vert_box)
{
   writer.write<std::uint32_t, std::uint32_t>(static_cast<std::uint32_t>(
                                                 vertex_buffer.size()),
                                              sizeof(Packed_output_terrain_vertex),
                                              packed_vertex_flags);

   const Vertex_position_compress pos_compress{vert_box};

   for (const auto& vertex : vertex_buffer) {
      writer.write(pack_vertex(vertex, pos_compress));
   }

   // Write padding to account for stride overshoot.
   writer.write(std::array<std::byte, 8>{});
}

}
