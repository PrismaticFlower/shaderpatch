#include "leaf_patch_world.hpp"

#include "../../game_support/structures/leaf_patch.hpp"
#include "../../logger.hpp"

namespace sp::shadows {

namespace {

struct Gpu_constants {
   glm::vec3 position_decompress_mul;
   std::uint32_t pad0;

   glm::vec3 position_decompress_add;
   std::uint32_t pad1;

   char pad[224];
};

const static std::uint32_t s_CompressedTexCoords[32] = {
   0x7FF0000, 0x7FF07FF, 0x0,       0x7FF,     //
   0x7FF07FF, 0x7FF0000, 0x7FF,     0x0,       //
   0x7FF0000, 0x7FF07FF, 0x0,       0x7FF,     //
   0x7FF07FF, 0x7FF0000, 0x7FF,     0x0,       //
   0x7FF0000, 0x7FF03FF, 0x4000000, 0x40003FF, //
   0x7FF0400, 0x7FF07FF, 0x4000400, 0x40007FF, //
   0x3FF0000, 0x3FF03FF, 0x0,       0x3FF,     //
   0x3FF0400, 0x3FF07FF, 0x400,     0x7FF,     //
};

#pragma pack(2)

struct Vertex_buffer_vertex {
   std::int16_t positionX;
   std::int16_t positionY;
   std::int16_t positionZ;
   std::uint32_t texcoords;
};

#pragma pack()

using Vertex_buffer_quad = std::array<Vertex_buffer_vertex, 4>;

auto make_particle_quad(const game_support::structures::LeafPatchClass_data& leaf_patch,
                        const game_support::structures::LeafPatchRenderable_data& renderable,
                        int particle_index) noexcept -> Vertex_buffer_quad
{
   using namespace game_support::structures;

   const LeafPatchParticle& particle = leaf_patch.m_particles[particle_index];
   const PblVector3* position = &particle.position;
   const float wiggle = particle.wiggle;
   const uint variation = particle.variation;
   const float size = particle.size;

   Vertex_buffer_quad quad;

   constexpr float vertexPosFactor = 1.0f / (50.0f * (1.0f / (32767.0f + 0.5f)));

   const float _size = size;
   const int vectorIndex = particle_index % 10;

   const float offsetX = renderable.offsetsXZ[vectorIndex].x * _size;
   const float offsetY = renderable.offsetsXZ[vectorIndex].y * _size;
   const float offsetZ = renderable.offsetsXZ[vectorIndex].z * _size;

   const float cameraZAxisX =
      position->x * vertexPosFactor + renderable.cameraZAxis.x * 0.1f;
   const float cameraZAxisY =
      position->y * vertexPosFactor + renderable.cameraZAxis.y * 0.1f;
   const float cameraZAxisZ =
      position->z * vertexPosFactor + renderable.cameraZAxis.z * 0.1f;

   const float wiggleOffset = wiggle - 0.5f;

   const float positionTopX = renderable.cameraYAxis.x * _size * 0.5f;
   const float positionTopY = renderable.cameraYAxis.y * _size * 0.5f;
   const float positionTopZ = renderable.cameraYAxis.z * _size * 0.5f;

   PblVector3 positionTopLeft;

   positionTopLeft.x = positionTopX + cameraZAxisX + offsetX * wiggleOffset;
   positionTopLeft.y = positionTopY + cameraZAxisY + offsetY * wiggleOffset;
   positionTopLeft.z = positionTopZ + cameraZAxisZ + offsetZ * wiggleOffset;

   PblVector3 positionBottomRight;

   positionBottomRight.x = (cameraZAxisX - positionTopX) - offsetX * 0.5f;
   positionBottomRight.y = (cameraZAxisY - positionTopY) - offsetY * 0.5f;
   positionBottomRight.z = (cameraZAxisZ - positionTopZ) - offsetZ * 0.5f;

   const uint vertLUTIndex = (variation & 0xff) + renderable.m_numPartsIs4 * 4;

   quad[0].positionX = (short)(int)positionTopLeft.x;
   quad[0].positionY = (short)(int)positionTopLeft.y;
   quad[0].positionZ = (short)(int)positionTopLeft.z;
   quad[0].texcoords = s_CompressedTexCoords[vertLUTIndex * 4 + 2];

   quad[1].positionX = (short)(int)(positionTopLeft.x + offsetX);
   quad[1].positionY = (short)(int)(offsetY + positionTopLeft.y);
   quad[1].positionZ = (short)(int)(offsetZ + positionTopLeft.z);
   quad[1].texcoords = s_CompressedTexCoords[vertLUTIndex * 4 + 3];

   quad[2].positionX = (short)(int)(positionBottomRight.x + offsetX);
   quad[2].positionY = (short)(int)(offsetY + positionBottomRight.y);
   quad[2].positionZ = (short)(int)(offsetZ + positionBottomRight.z);
   quad[2].texcoords = s_CompressedTexCoords[vertLUTIndex * 4 + 1];

   quad[3].positionX = (short)(int)positionBottomRight.x;
   quad[3].positionY = (short)(int)positionBottomRight.y;
   quad[3].positionZ = (short)(int)positionBottomRight.z;
   quad[3].texcoords = s_CompressedTexCoords[vertLUTIndex * 4];

   return quad;
}

}

Leaf_patch_world::Leaf_patch_world(ID3D11Device& device)
{
   const Gpu_constants constants{
      .position_decompress_mul = glm::vec3{(50.0f - -50.0f) * (0.5f / INT16_MAX)},
      .position_decompress_add = glm::vec3{(50.0f + -50.0f) * 0.5f},
   };
   const D3D11_BUFFER_DESC desc{.ByteWidth = sizeof(Gpu_constants),
                                .Usage = D3D11_USAGE_IMMUTABLE,
                                .BindFlags = D3D11_BIND_CONSTANT_BUFFER};

   const D3D11_SUBRESOURCE_DATA init{.pSysMem = &constants};

   if (FAILED(device.CreateBuffer(&desc, &init, constants_buffer.clear_and_assign()))) {
      log_and_terminate("Failed to create leaf patch constants buffer!");
   }
}

void Leaf_patch_world::update_from_game(ID3D11Device& device, ID3D11DeviceContext2& dc,
                                        Texture_table& textures) noexcept
{
   std::swap(previous_game_leaf_patch_classes, game_leaf_patch_classes);

   game_support::acquire_leaf_patches(game_leaf_patches, game_leaf_patch_classes);

   if (game_leaf_patches.empty() || game_leaf_patch_classes.empty()) return;

   if (game_leaf_patch_classes != previous_game_leaf_patch_classes) {
      UINT new_total_particles = 0;
      UINT new_max_instance_particles = 0;

      classes.clear();
      classes.reserve(game_leaf_patch_classes.size());

      for (INT base_vertex = 0;
           const game_support::structures::LeafPatchClass_data* leaf_patch_class :
           game_leaf_patch_classes) {
         const UINT num_particles =
            static_cast<UINT>(leaf_patch_class->m_numParticles);

         new_total_particles += num_particles;
         new_max_instance_particles =
            std::max(num_particles, new_max_instance_particles);

         classes.push_back({
            .index_count = num_particles * 6u,
            .base_vertex = base_vertex,

            .texture_index = textures.acquire(leaf_patch_class->m_textureHash),
         });

         base_vertex += num_particles * 4u;
      }

      if (new_total_particles == 0) return;

      if (max_total_particles < new_total_particles) {
         const D3D11_BUFFER_DESC desc{
            .ByteWidth = sizeof(Vertex_buffer_quad) * new_total_particles,
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_VERTEX_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
         };

         if (FAILED(device.CreateBuffer(&desc, nullptr,
                                        vertex_buffer.clear_and_assign()))) {
            log_and_terminate("Failed to create leaf patches vertex buffer!");
         }

         max_total_particles = new_total_particles;
      }

      if (max_instance_particles < new_max_instance_particles) {
         const D3D11_BUFFER_DESC desc{
            .ByteWidth = sizeof(std::uint16_t) * 6 * new_max_instance_particles,
            .Usage = D3D11_USAGE_IMMUTABLE,
            .BindFlags = D3D11_BIND_INDEX_BUFFER,
         };

         std::vector<std::uint16_t> indices;
         indices.reserve(6 * new_max_instance_particles);

         for (std::uint16_t i = 0; i < new_total_particles; ++i) {
            const std::uint16_t base_vertex = i * 4 + 2;

            indices.push_back(base_vertex - std::uint16_t{2});
            indices.push_back(base_vertex - std::uint16_t{1});
            indices.push_back(base_vertex);
            indices.push_back(base_vertex - std::uint16_t{2});
            indices.push_back(base_vertex);
            indices.push_back(base_vertex + std::uint16_t{1});
         }

         const D3D11_SUBRESOURCE_DATA init_data{
            .pSysMem = indices.data(),
         };

         if (FAILED(device.CreateBuffer(&desc, &init_data,
                                        index_buffer.clear_and_assign()))) {
            log_and_terminate("Failed to create leaf patches index buffer!");
         }

         max_instance_particles = new_max_instance_particles;
      }
   }

   D3D11_MAPPED_SUBRESOURCE mapped_vertex_buffer;

   if (FAILED(dc.Map(vertex_buffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0,
                     &mapped_vertex_buffer))) {
      log_and_terminate("Failed to map vertex buffer for leaf patches.");
   }

   Vertex_buffer_quad* next_quad =
      static_cast<Vertex_buffer_quad*>(mapped_vertex_buffer.pData);

   for (std::size_t class_index = 0; class_index < classes.size(); ++class_index) {
      classes[class_index].bounding_spheres.clear();
      classes[class_index].transforms.clear();

      const game_support::structures::LeafPatchClass_data* leaf_patch_class =
         game_leaf_patch_classes[class_index];

      for (int particle_index = 0;
           particle_index < leaf_patch_class->m_numParticles; ++particle_index) {
         Vertex_buffer_quad quad =
            make_particle_quad(*leaf_patch_class,
                               leaf_patch_class->m_renderable->LeafPatchRenderable_data,
                               particle_index);

         std::memcpy(next_quad, &quad, sizeof(Vertex_buffer_quad));

         next_quad += 1;
      }
   }

   dc.Unmap(vertex_buffer.get(), 0);

   if (max_total_instances < game_leaf_patches.size()) {
      const D3D11_BUFFER_DESC desc{
         .ByteWidth = sizeof(std::array<glm::vec4, 3>) * game_leaf_patches.size(),
         .Usage = D3D11_USAGE_DYNAMIC,
         .BindFlags = D3D11_BIND_VERTEX_BUFFER,
         .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
      };

      if (FAILED(device.CreateBuffer(&desc, nullptr,
                                     instances_buffer.clear_and_assign()))) {
         log_and_terminate("Failed to create leaf patches instance buffer!");
      }

      max_total_instances = game_leaf_patches.size();
   }

   for (const game_support::Leaf_patch& patch : game_leaf_patches) {
      const float height_scale =
         game_leaf_patch_classes[patch.class_index]->m_heightScale;

      classes[patch.class_index].bounding_spheres.push_back({
         .position = {patch.transform[0].w, patch.transform[1].w,
                      patch.transform[2].w},
         .radius = patch.bounds_radius,
      });
      classes[patch.class_index].transforms.push_back(patch.transform);

      std::array<glm::vec4, 3>& transform =
         classes[patch.class_index].transforms.back();

      transform[0].y *= height_scale;
      transform[1].y *= height_scale;
      transform[2].y *= height_scale;
   }
}

}