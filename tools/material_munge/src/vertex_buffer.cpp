
#include "vertex_buffer.hpp"

#include <limits>
#include <tuple>

namespace sp {

namespace {

class Position_decompress {
public:
   Position_decompress(const std::array<glm::vec3, 2> vert_box) noexcept
   {
      low = vert_box[0];
      mul = (vert_box[1] - vert_box[0]);
   }

   glm::vec3 operator()(const glm::i16vec4 compressed) const noexcept
   {
      const auto c = static_cast<glm::vec3>(compressed);
      constexpr float i16min = std::numeric_limits<glm::int16>::min();
      constexpr float i16max = std::numeric_limits<glm::int16>::max();

      return low + (c - i16min) * mul / (i16max - i16min);
   }

private:
   glm::vec3 low;
   glm::vec3 mul;
};

class Position_compress {
public:
   Position_compress(const std::array<glm::vec3, 2> vert_box) noexcept
   {
      min = vert_box[0];
      max = vert_box[1];
      div = (vert_box[1] - vert_box[0]);
   }

   glm::i16vec4 operator()(const glm::vec3 pos) const noexcept
   {
      constexpr float i16min = std::numeric_limits<glm::int16>::min();
      constexpr float i16max = std::numeric_limits<glm::int16>::max();

      const auto clamped = glm::clamp(pos, min, max);
      const auto compressed = i16min + (pos - min) * (i16max - i16min) / div;

      return {static_cast<glm::i16vec3>(compressed), 0};
   }

private:
   glm::vec3 min;
   glm::vec3 max;
   glm::vec3 div;
};

auto decompress_normal(const glm::uint32 normal) noexcept -> glm::vec3
{
   const auto swizzled_normal =
      static_cast<glm::vec3>(glm::unpackUnorm4x8(normal)) * 2.0f - 1.0f;

   return {swizzled_normal.z, swizzled_normal.y, swizzled_normal.x};
}

auto compress_normal(const glm::vec3 normal, const float w = 0.0f) noexcept -> glm::uint32
{
   const auto swizzled_normal = glm::vec3{normal.z, normal.y, normal.x};

   return glm::packUnorm4x8(glm::vec4{swizzled_normal * 0.5f + 0.5f, w});
}

auto compress_texcoords(const glm::vec2 texcoords) noexcept -> glm::i16vec2
{
   constexpr float i16min = std::numeric_limits<glm::int16>::min();
   constexpr float i16max = std::numeric_limits<glm::int16>::max();

   const auto clamped =
      glm::clamp(texcoords * 2048.f, glm::vec2{i16min}, glm::vec2{i16max});

   return static_cast<glm::i16vec2>(clamped);
}

void read_vertex(const Vbuf_flags flags, ucfb::Reader_strict<"VBUF"_mn>& vbuf,
                 const std::size_t index, const Position_decompress& pos_decompress,
                 Vertex_buffer& output) noexcept
{
   if ((flags & Vbuf_flags::position) == Vbuf_flags::position) {
      if ((flags & Vbuf_flags::position_compressed) == Vbuf_flags::position_compressed) {
         const auto compressed = vbuf.read<glm::i16vec4>();

         output.positions[index] = pos_decompress(compressed);
      }
      else {
         output.positions[index] = vbuf.read<glm::vec3>();
      }
   }

   if ((flags & Vbuf_flags::blendindices) == Vbuf_flags::blendindices)
      output.blendindices[index] = vbuf.read<glm::uint32>();

   if ((flags & Vbuf_flags::normal) == Vbuf_flags::normal) {
      if ((flags & Vbuf_flags::normal_compressed) == Vbuf_flags::normal_compressed) {
         const auto compressed = vbuf.read<glm::uint32>();

         output.normals[index] = decompress_normal(compressed);
      }
      else {
         output.normals[index] = vbuf.read<glm::vec3>();
      }
   }

   if ((flags & Vbuf_flags::tangents) == Vbuf_flags::tangents) {
      if ((flags & Vbuf_flags::normal_compressed) == Vbuf_flags::normal_compressed) {
         const auto tangent_compressed = vbuf.read<glm::uint32>();
         const auto binormal_compressed = vbuf.read<glm::uint32>();

         output.tangents[index] = decompress_normal(tangent_compressed);
         output.binormals[index] = decompress_normal(binormal_compressed);
      }
      else {
         output.tangents[index] = vbuf.read<glm::vec3>();
         output.binormals[index] = vbuf.read<glm::vec3>();
      }
   }

   if ((flags & Vbuf_flags::color) == Vbuf_flags::color)
      output.colors[index] = vbuf.read<glm::uint32>();

   if ((flags & Vbuf_flags::static_lighting) == Vbuf_flags::static_lighting)
      output.static_lighting_colors[index] = vbuf.read<glm::uint32>();

   if ((flags & Vbuf_flags::texcoords) == Vbuf_flags::texcoords) {
      if ((flags & Vbuf_flags::texcoord_compressed) == Vbuf_flags::texcoord_compressed) {
         const auto compressed = static_cast<glm::vec2>(vbuf.read<glm::i16vec2>());

         output.texcoords[index] = compressed / 2048.f;
      }
      else {
         output.texcoords[index] = vbuf.read<glm::vec2>();
      }
   }
}

void write_vertex(const Vertex_buffer& vertex_buffer, const int index,
                  const Vbuf_flags flags, const Position_compress& pos_compress,
                  ucfb::Editor_data_writer& writer) noexcept
{
   if ((flags & Vbuf_flags::position) == Vbuf_flags::position) {
      if ((flags & Vbuf_flags::position_compressed) == Vbuf_flags::position_compressed) {
         writer.write(pos_compress(vertex_buffer.positions[index]));
      }
      else {
         writer.write(vertex_buffer.positions[index]);
      }
   }

   if ((flags & Vbuf_flags::blendindices) == Vbuf_flags::blendindices)
      writer.write(vertex_buffer.blendindices[index]);

   if ((flags & Vbuf_flags::normal) == Vbuf_flags::normal) {
      if ((flags & Vbuf_flags::normal_compressed) == Vbuf_flags::normal_compressed) {
         writer.write(compress_normal(vertex_buffer.normals[index]));
      }
      else {
         writer.write(vertex_buffer.normals[index]);
      }
   }

   if ((flags & Vbuf_flags::tangents) == Vbuf_flags::tangents) {
      if ((flags & Vbuf_flags::normal_compressed) == Vbuf_flags::normal_compressed) {
         writer.write(compress_normal(vertex_buffer.tangents[index],
                                      vertex_buffer.bitangent_signs
                                         ? vertex_buffer.bitangent_signs[index]
                                         : 0.0f));

         if (vertex_buffer.binormals)
            writer.write(compress_normal(vertex_buffer.binormals[index]));
         else
            writer.write(glm::uint32{});
      }
      else {
         writer.write(vertex_buffer.tangents[index]);

         if (vertex_buffer.binormals)
            writer.write(vertex_buffer.binormals[index]);
         else if (vertex_buffer.binormals && vertex_buffer.bitangent_signs)
            writer.write(glm::vec3{glm::vec2{vertex_buffer.binormals[index]},
                                   vertex_buffer.bitangent_signs[index]});
         else if (vertex_buffer.bitangent_signs)
            writer.write(glm::vec3{0.0f, 0.0f, vertex_buffer.bitangent_signs[index]});
         else
            writer.write(glm::vec3{});
      }
   }

   if ((flags & Vbuf_flags::color) == Vbuf_flags::color)
      writer.write(vertex_buffer.colors[index]);

   if ((flags & Vbuf_flags::static_lighting) == Vbuf_flags::static_lighting)
      writer.write(vertex_buffer.static_lighting_colors[index]);

   if ((flags & Vbuf_flags::texcoords) == Vbuf_flags::texcoords) {
      if ((flags & Vbuf_flags::texcoord_compressed) == Vbuf_flags::texcoord_compressed)
         writer.write(compress_texcoords(vertex_buffer.texcoords[index]));
      else
         writer.write(vertex_buffer.texcoords[index]);
   }
}

auto get_vbuf_flags(const Vertex_buffer& vertex_buffer, const bool compressed) noexcept
   -> Vbuf_flags
{
   Vbuf_flags flags{};

   if (vertex_buffer.positions) {
      flags |= Vbuf_flags::position;

      if (compressed) flags |= Vbuf_flags::position_compressed;
   }

   if (vertex_buffer.blendindices) flags |= Vbuf_flags::blendindices;

   if (vertex_buffer.normals) {
      flags |= Vbuf_flags::normal;

      if (compressed) flags |= Vbuf_flags::normal_compressed;
   }

   if (vertex_buffer.tangents || vertex_buffer.bitangent_signs ||
       vertex_buffer.binormals) {
      flags |= Vbuf_flags::tangents;

      if (compressed) flags |= Vbuf_flags::normal_compressed;
   }

   if (vertex_buffer.colors && !vertex_buffer.static_lighting_colors)
      flags |= Vbuf_flags::color;
   if (vertex_buffer.static_lighting_colors)
      flags |= Vbuf_flags::static_lighting;

   if (vertex_buffer.texcoords) {
      flags |= Vbuf_flags::texcoords;

      if (compressed) flags |= Vbuf_flags::texcoord_compressed;
   }

   return flags;
}

auto get_vbuf_stride(const Vbuf_flags flags) noexcept -> std::uint32_t
{
   std::uint32_t stride{};

   if ((flags & Vbuf_flags::position) == Vbuf_flags::position) {
      if ((flags & Vbuf_flags::position_compressed) == Vbuf_flags::position_compressed)
         stride += sizeof(glm::i16vec4);
      else
         stride += sizeof(glm::vec3);
   }

   if ((flags & Vbuf_flags::blendindices) == Vbuf_flags::blendindices)
      stride += sizeof(glm::uint32);

   if ((flags & Vbuf_flags::normal) == Vbuf_flags::normal) {
      if ((flags & Vbuf_flags::normal_compressed) == Vbuf_flags::normal_compressed) {
         stride += sizeof(glm::uint32);
      }
      else {
         stride += sizeof(glm::vec3);
      }
   }

   if ((flags & Vbuf_flags::tangents) == Vbuf_flags::tangents) {
      if ((flags & Vbuf_flags::normal_compressed) == Vbuf_flags::normal_compressed) {
         stride += sizeof(glm::uint32) * 2;
      }
      else {
         stride += sizeof(glm::vec3) * 2;
      }
   }

   if ((flags & Vbuf_flags::color) == Vbuf_flags::color)
      stride += sizeof(glm::uint32);

   if ((flags & Vbuf_flags::static_lighting) == Vbuf_flags::static_lighting)
      stride += sizeof(glm::uint32);

   if ((flags & Vbuf_flags::texcoords) == Vbuf_flags::texcoords) {
      if ((flags & Vbuf_flags::texcoord_compressed) == Vbuf_flags::texcoord_compressed) {
         stride += sizeof(glm::i16vec2);
      }
      else {
         stride += sizeof(glm::vec2);
      }
   }

   return stride;
}
}

Vertex_buffer::Vertex_buffer(const std::size_t count, const Vbuf_flags flags) noexcept
   : count{count}
{
   if ((flags & Vbuf_flags::position) == Vbuf_flags::position)
      positions = std::make_unique<glm::vec3[]>(count);

   if ((flags & Vbuf_flags::blendindices) == Vbuf_flags::blendindices)
      blendindices = std::make_unique<glm::uint32[]>(count);

   if ((flags & Vbuf_flags::normal) == Vbuf_flags::normal)
      normals = std::make_unique<glm::vec3[]>(count);

   if ((flags & Vbuf_flags::tangents) == Vbuf_flags::tangents) {
      tangents = std::make_unique<glm::vec3[]>(count);
      binormals = std::make_unique<glm::vec3[]>(count);
   }

   if ((flags & Vbuf_flags::color) == Vbuf_flags::color)
      colors = std::make_unique<glm::uint32[]>(count);

   if ((flags & Vbuf_flags::static_lighting) == Vbuf_flags::static_lighting)
      static_lighting_colors = std::make_unique<glm::uint32[]>(count);

   if ((flags & Vbuf_flags::texcoords) == Vbuf_flags::texcoords)
      texcoords = std::make_unique<glm::vec2[]>(count);
}

auto create_vertex_buffer(ucfb::Reader_strict<"VBUF"_mn> vbuf,
                          const std::array<glm::vec3, 2> vert_box) -> Vertex_buffer
{
   const auto [count, stride, flags] =
      vbuf.read_multi<std::uint32_t, std::uint32_t, Vbuf_flags>();

   if ((flags & Vbuf_flags::blendweight) == Vbuf_flags::blendweight) {
      throw std::runtime_error{
         "VBUF has softskin weights! SWBFII does not have support for "
         "hardware "
         "accelerated mesh soft skining, look for \"-softskin\" in any "
         "relevent `.option` files and remove it. "};
   }

   Vertex_buffer buffer{count, flags};

   const Position_decompress pos_decompress{vert_box};

   for (auto i = 0u; i < count; ++i) {
      read_vertex(flags, vbuf, i, pos_decompress, buffer);
   }

   return buffer;
}

void output_vertex_buffer(const Vertex_buffer& vertex_buffer,
                          ucfb::Editor_data_writer& writer, const bool compressed,
                          const std::array<glm::vec3, 2> vert_box)
{
   const auto flags = get_vbuf_flags(vertex_buffer, compressed);
   const auto stride = get_vbuf_stride(flags);

   writer.write(static_cast<std::uint32_t>(vertex_buffer.count), stride, flags);

   const Position_compress pos_compress{vert_box};

   for (auto i = 0; i < vertex_buffer.count; ++i) {
      write_vertex(vertex_buffer, i, flags, pos_compress, writer);
   }
}
}
