
#include "optimize_mesh.hpp"

#include <utility>

#include <DirectXMesh.h>

namespace sp {

static_assert(sizeof(std::array<std::array<std::uint16_t, 3>, 2>) == 12);

namespace {

auto init_dest_vertex_buffer(const Vertex_buffer& old) noexcept -> Vertex_buffer
{
   Vertex_buffer vertex_buffer{};

   vertex_buffer.count = old.count;

   if (old.positions)
      vertex_buffer.positions = std::make_unique<glm::vec3[]>(old.count);

   if (old.blendindices)
      vertex_buffer.blendindices = std::make_unique<glm::uint32[]>(old.count);

   if (old.blendweights)
      vertex_buffer.blendweights = std::make_unique<glm::vec3[]>(old.count);

   if (old.normals)
      vertex_buffer.normals = std::make_unique<glm::vec3[]>(old.count);

   if (old.tangents)
      vertex_buffer.tangents = std::make_unique<glm::vec3[]>(old.count);

   if (old.bitangent_signs)
      vertex_buffer.bitangent_signs = std::make_unique<float[]>(old.count);

   if (old.binormals)
      vertex_buffer.binormals = std::make_unique<glm::vec3[]>(old.count);

   if (old.colors)
      vertex_buffer.colors = std::make_unique<glm::uint32[]>(old.count);

   if (old.static_lighting_colors)
      vertex_buffer.static_lighting_colors =
         std::make_unique<glm::uint32[]>(old.count);

   if (old.texcoords)
      vertex_buffer.texcoords = std::make_unique<glm::vec2[]>(old.count);

   return vertex_buffer;
}

auto optimize_index_buffer(Index_buffer_16 index_buffer)
   -> std::vector<std::array<std::uint16_t, 3>>
{
   std::vector<std::uint32_t> remap;
   remap.resize(index_buffer.size());

   if (const auto result =
          DirectX::OptimizeFacesLRU(index_buffer[0].data(), index_buffer.size(),
                                    remap.data());
       FAILED(result)) {
      std::terminate();
   }

   if (const auto result = DirectX::ReorderIB(index_buffer[0].data(),
                                              index_buffer.size(), remap.data());
       FAILED(result)) {
      std::terminate();
   }

   return index_buffer;
}

auto optimize_vertex_buffer(Index_buffer_16 index_buffer, const Vertex_buffer& vertex_buffer)
   -> std::pair<std::vector<std::array<std::uint16_t, 3>>, Vertex_buffer>
{
   std::vector<std::uint32_t> remap;
   remap.resize(vertex_buffer.count);

   if (const auto result =
          DirectX::OptimizeVertices(index_buffer[0].data(), index_buffer.size(),
                                    vertex_buffer.count, remap.data());
       FAILED(result)) {
      std::terminate();
   }

   if (const auto result =
          DirectX::FinalizeIB(index_buffer[0].data(), index_buffer.size(),
                              remap.data(), vertex_buffer.count);
       FAILED(result)) {
      std::terminate();
   }

   auto dest_vertex_buffer = init_dest_vertex_buffer(vertex_buffer);

   for (auto i = 0; i < vertex_buffer.count; ++i) {
      if (vertex_buffer.positions)
         dest_vertex_buffer.positions[i] = vertex_buffer.positions[remap[i]];

      if (vertex_buffer.blendindices)
         dest_vertex_buffer.blendindices[i] = vertex_buffer.blendindices[remap[i]];

      if (vertex_buffer.blendweights)
         dest_vertex_buffer.blendweights[i] = vertex_buffer.blendweights[remap[i]];

      if (vertex_buffer.normals)
         dest_vertex_buffer.normals[i] = vertex_buffer.normals[remap[i]];

      if (vertex_buffer.tangents)
         dest_vertex_buffer.tangents[i] = vertex_buffer.tangents[remap[i]];

      if (vertex_buffer.bitangent_signs)
         dest_vertex_buffer.bitangent_signs[i] =
            vertex_buffer.bitangent_signs[remap[i]];

      if (vertex_buffer.binormals)
         dest_vertex_buffer.binormals[i] = vertex_buffer.binormals[remap[i]];

      if (vertex_buffer.colors)
         dest_vertex_buffer.colors[i] = vertex_buffer.colors[remap[i]];

      if (vertex_buffer.static_lighting_colors)
         dest_vertex_buffer.static_lighting_colors[i] =
            vertex_buffer.static_lighting_colors[remap[i]];

      if (vertex_buffer.texcoords)
         dest_vertex_buffer.texcoords[i] = vertex_buffer.texcoords[remap[i]];
   }

   return {std::move(index_buffer), std::move(dest_vertex_buffer)};
}

}

auto optimize_mesh(const Index_buffer_16& index_buffer,
                   const Vertex_buffer& vertex_buffer) noexcept
   -> std::pair<Index_buffer_16, Vertex_buffer>
{
   return optimize_vertex_buffer(optimize_index_buffer(index_buffer), vertex_buffer);
}

}
