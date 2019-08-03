
#include "generate_tangents.hpp"
#include "mikktspace/mikktspace.h"
#include "weld_vertex_list.hpp"

namespace sp {

namespace {
auto init_tangents_vertex_buffer(const std::vector<std::array<std::uint16_t, 3>>& index_buffer,
                                 const Vertex_buffer& old_vbuf) noexcept -> Vertex_buffer
{
   Vertex_buffer vbuf;

   vbuf.count = index_buffer.size() * 3;

   vbuf.positions = std::make_unique<glm::vec3[]>(vbuf.count);
   vbuf.normals = std::make_unique<glm::vec3[]>(vbuf.count);
   vbuf.tangents = std::make_unique<glm::vec3[]>(vbuf.count);
   vbuf.bitangent_signs = std::make_unique<float[]>(vbuf.count);
   vbuf.texcoords = std::make_unique<glm::vec2[]>(vbuf.count);

   if (old_vbuf.blendindices)
      vbuf.blendindices = std::make_unique<glm::uint32[]>(vbuf.count);
   if (old_vbuf.colors)
      vbuf.colors = std::make_unique<glm::uint32[]>(vbuf.count);
   if (old_vbuf.static_lighting_colors)
      vbuf.static_lighting_colors = std::make_unique<glm::uint32[]>(vbuf.count);

   for (auto f = 0; f < index_buffer.size(); ++f) {
      for (auto v = 0; v < 3; ++v) {
         vbuf.positions[f * 3 + v] = old_vbuf.positions[index_buffer[f][v]];
         vbuf.normals[f * 3 + v] = old_vbuf.normals[index_buffer[f][v]];
         vbuf.texcoords[f * 3 + v] = old_vbuf.texcoords[index_buffer[f][v]];

         if (old_vbuf.blendindices)
            vbuf.blendindices[f * 3 + v] = old_vbuf.blendindices[index_buffer[f][v]];
         if (old_vbuf.colors)
            vbuf.colors[f * 3 + v] = old_vbuf.colors[index_buffer[f][v]];
         if (old_vbuf.static_lighting_colors)
            vbuf.static_lighting_colors[f * 3 + v] =
               old_vbuf.static_lighting_colors[index_buffer[f][v]];
      }
   }

   return vbuf;
}

constexpr SMikkTSpaceInterface mikktspace_interface{
   // m_getNumFaces
   [](const SMikkTSpaceContext* context) -> int {
      return static_cast<int>(static_cast<Vertex_buffer*>(context->m_pUserData)->count) / 3;
   },

   // m_getNumVerticesOfFace
   [](const SMikkTSpaceContext*, const int) -> int { return 3; },

   // m_getPosition
   [](const SMikkTSpaceContext* context, float pos_out[], const int face, const int vert) {
      auto& vbuf = *static_cast<Vertex_buffer*>(context->m_pUserData);

      std::memcpy(pos_out, &vbuf.positions[face * 3 + vert], sizeof(glm::vec3));
   },

   // m_getNormal
   [](const SMikkTSpaceContext* context, float norm_out[], const int face, const int vert) {
      auto& vbuf = *static_cast<Vertex_buffer*>(context->m_pUserData);

      std::memcpy(norm_out, &vbuf.normals[face * 3 + vert], sizeof(glm::vec3));
   },

   // m_getTexCoord
   [](const SMikkTSpaceContext* context, float texcoord_out[], const int face,
      const int vert) {
      auto& vbuf = *static_cast<Vertex_buffer*>(context->m_pUserData);

      std::memcpy(texcoord_out, &vbuf.texcoords[face * 3 + vert], sizeof(glm::vec2));
   },

   // m_setTSpaceBasic
   [](const SMikkTSpaceContext* context, const float tangent[],
      const float sign, const int face, const int vert) {
      auto& vbuf = *static_cast<Vertex_buffer*>(context->m_pUserData);

      std::memcpy(&vbuf.tangents[face * 3 + vert], tangent, sizeof(glm::vec3));
      vbuf.bitangent_signs[face * 3 + vert] = sign;
   },

   nullptr};

}

auto generate_tangents(const std::vector<std::array<std::uint16_t, 3>>& index_buffer,
                       const Vertex_buffer& vertex_buffer) noexcept
   -> std::pair<std::vector<std::array<std::uint16_t, 3>>, Vertex_buffer>
{
   Expects(vertex_buffer.positions && vertex_buffer.normals && vertex_buffer.texcoords);

   auto results_vbuf = init_tangents_vertex_buffer(index_buffer, vertex_buffer);

   auto interface = mikktspace_interface;
   const SMikkTSpaceContext context{&interface, &results_vbuf};

   genTangSpaceDefault(&context);

   return weld_vertex_list(results_vbuf);
}

}
