#include "read_model.hpp"
#include "material_flags.hpp"

#include <optional>

#pragma warning(disable : 4063)

#include <d3d9.h>

namespace sp::shadows {

namespace {

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

constexpr bool marked_as_enum_flag(Vbuf_flags) noexcept
{
   return true;
}

auto get_vbuf_texcoords_offset(const Vbuf_flags flags) noexcept -> std::uint32_t
{
   std::uint32_t offset{};

   if ((flags & Vbuf_flags::position) == Vbuf_flags::position) {
      if ((flags & Vbuf_flags::position_compressed) == Vbuf_flags::position_compressed)
         offset += sizeof(glm::i16vec4);
      else
         offset += sizeof(glm::vec3);
   }

   if ((flags & Vbuf_flags::blendindices) == Vbuf_flags::blendindices)
      offset += sizeof(glm::uint32);

   if ((flags & Vbuf_flags::blendweight) == Vbuf_flags::blendweight) {
      if ((flags & Vbuf_flags::blendinfo_compressed) == Vbuf_flags::blendinfo_compressed) {
         offset += sizeof(glm::uint32);
      }
      else {
         offset += sizeof(glm::vec2);
      }
   }

   if ((flags & Vbuf_flags::normal) == Vbuf_flags::normal) {
      if ((flags & Vbuf_flags::normal_compressed) == Vbuf_flags::normal_compressed) {
         offset += sizeof(glm::uint32);
      }
      else {
         offset += sizeof(glm::vec3);
      }
   }

   if ((flags & Vbuf_flags::tangents) == Vbuf_flags::tangents) {
      if ((flags & Vbuf_flags::normal_compressed) == Vbuf_flags::normal_compressed) {
         offset += sizeof(glm::uint32) * 2;
      }
      else {
         offset += sizeof(glm::vec3) * 2;
      }
   }

   if ((flags & Vbuf_flags::color) == Vbuf_flags::color)
      offset += sizeof(glm::uint32);

   if ((flags & Vbuf_flags::static_lighting) == Vbuf_flags::static_lighting)
      offset += sizeof(glm::uint32);

   return offset;
}

struct Model_info {
   std::uint32_t unknown0 = 0;
   std::uint32_t unknown1 = 0;
   std::uint32_t segment_count = 0;
   std::uint32_t unknown2 = 0;
   glm::vec3 vertex_aabb_min = {};
   glm::vec3 vertex_aabb_max = {};
   glm::vec3 visibility_aabb_min = {};
   glm::vec3 visibility_aabb_max = {};
   std::uint32_t unknown3 = 1;
   std::uint32_t total_vertex_count = 0;
};

struct Segment_info {
   D3DPRIMITIVETYPE primitive_type = D3DPRIMITIVETYPE::D3DPT_TRIANGLELIST;
   std::uint32_t vertex_count = 0;
   std::uint32_t primitive_count = 0;
};

struct Material {
   Material_flags flags = Material_flags::normal;
   std::uint32_t diffuse_color = 0xff'ff'ff'ffu;
   std::uint32_t specular_color = 0xff'ff'ff'ffu;
   std::uint32_t specular_exponent = 50;
   std::uint32_t data0 = 0;
   std::uint32_t data1 = 0;
};

auto read_segment(ucfb::File_reader_child& segm) -> std::optional<Input_model::Segment>
{
   Input_model::Segment result;

   while (segm) {
      auto child = segm.read_child();

      switch (child.magic_number()) {
      case "INFO"_mn: {
         const Segment_info segment_info = child.read<Segment_info>();

         switch (segment_info.primitive_type) {
         case D3DPT_TRIANGLELIST: {
            result.topology = Input_model::Topology::tri_list;
         } break;
         case D3DPT_TRIANGLESTRIP: {
            result.topology = Input_model::Topology::tri_strip;
         } break;
         default:
            return std::nullopt;
         }

         result.vertex_count = segment_info.vertex_count;
         result.primitive_count = segment_info.primitive_count;

      } break;
      case "MTRL"_mn: {
         const Material material = child.read<Material>();

         // followed by attached light name as a string

         result.hardedged = is_flag_set(material.flags, Material_flags::hardedged);
         result.doublesided = is_flag_set(material.flags, Material_flags::doublesided);

         if (is_flag_set(material.flags, Material_flags::transparent)) {
            return std::nullopt;
         }
      } break;
      case "RTYP"_mn: {
         // Might not be needed, string
      } break;
      case "MNAM"_mn: {
         // Not needed, string
      } break;
      case "TNAM"_mn: {
         const std::uint32_t texture_index = child.read<uint32_t>();

         if (texture_index == 0 && result.hardedged) {
            result.diffuse_texture = child.read_string();
         }
      } break;
      case "IBUF"_mn: {
         const std::uint32_t index_count = child.read<uint32_t>();

         result.indices.resize(index_count);

         child.read_array<std::uint16_t>(result.indices);
      } break;
      case "VBUF"_mn: {
         if (!result.vertices.empty()) continue;

         const std::uint32_t count = child.read<std::uint32_t>();
         const std::uint32_t stride = child.read<std::uint32_t>();
         const Vbuf_flags flags = child.read<Vbuf_flags>();

         if (!is_flag_set(flags, Vbuf_flags::position_compressed)) continue;
         if (stride < sizeof(std::array<std::int16_t, 3>)) continue;

         result.vertices.reserve(count);

         if (result.hardedged) {
            if (!is_flag_set(flags, Vbuf_flags::texcoords)) continue;

            result.texcoords.reserve(count);

            const std::uint32_t texcoords_size =
               is_flag_set(flags, Vbuf_flags::texcoord_compressed)
                  ? sizeof(std::array<std::int16_t, 2>)
                  : sizeof(glm::vec2);

            if (stride < (sizeof(std::array<std::int16_t, 3>) + texcoords_size))
               continue;

            const std::uint32_t skip_amount =
               stride - sizeof(std::array<std::int16_t, 3>) - texcoords_size;

            for (std::uint32_t i = 0; i < count; ++i) {
               result.vertices.push_back(child.read<std::array<std::int16_t, 3>>());

               child.consume(skip_amount);

               if (is_flag_set(flags, Vbuf_flags::texcoord_compressed)) {
                  result.texcoords.push_back(child.read<std::array<std::int16_t, 2>>());
               }
               else {
                  const glm::vec2 texcoords = child.read<glm::vec2>();

                  result.texcoords.push_back(
                     {static_cast<std::int16_t>(2048.0f * texcoords.x),
                      static_cast<std::int16_t>(2048.0f * texcoords.y)});
               }
            }
         }
         else {
            const std::uint32_t skip_amount =
               stride - sizeof(std::array<std::int16_t, 3>);

            for (std::uint32_t i = 0; i < count; ++i) {
               result.vertices.push_back(child.read<std::array<std::int16_t, 3>>());

               child.consume(skip_amount);
            }
         }

         // complex
      } break;
      case "BNAM"_mn: {
         // parent bone name
      } break;
      case "BMAP"_mn: {
         // bone map/envelope for skinned meshes uint32 elem count followed by uint8[elem count] array

         return std::nullopt;
      } break;
      }
   }

   if (result.vertices.empty()) return std::nullopt;
   if (result.indices.empty()) return std::nullopt;
   if (result.vertex_count == 0) return std::nullopt;
   if (result.primitive_count == 0) return std::nullopt;

   return result;
}

}

auto read_model(ucfb::File_reader_child& modl) -> Input_model
{
   Input_model result;
   Model_info model_info;

   while (modl) {
      auto child = modl.read_child();

      switch (child.magic_number()) {
      case "NAME"_mn: {
         result.name = child.read_string();
      } break;
      case "VRTX"_mn: {
         // Not needed, single uint32
      } break;
      case "NODE"_mn: {
         // Parent bone, string
      } break;
      case "INFO"_mn: {
         model_info = child.read<Model_info>();

         result.min_vertex = model_info.vertex_aabb_min;
         result.max_vertex = model_info.vertex_aabb_max;
         result.segments.reserve(model_info.segment_count);
      } break;
      case "segm"_mn: {
         std::optional<Input_model::Segment> segment = read_segment(child);

         if (segment) result.segments.push_back(std::move(segment.value()));
      } break;
      case "SPHR"_mn: {
         // Not needed, float3 position followed by float radius
      } break;
      }
   }

   return result;
}

}