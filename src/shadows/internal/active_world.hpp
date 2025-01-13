#pragma once

#include "com_ptr.hpp"
#include "world/game_model.hpp"
#include "world/model.hpp"
#include "world/object_instance.hpp"

#include <memory>
#include <span>
#include <vector>

#include <d3d11_2.h>

#include <glm/glm.hpp>

#include <immintrin.h>

namespace sp::shadows {

struct Aligned_delete {
   void operator()(void* ptr) const noexcept;
};

template<typename T>
using simd_array_ptr = std::unique_ptr<float, Aligned_delete>;

struct Instance_list {
   std::uint32_t instance_count = 0;

   simd_array_ptr<float> bbox_min_x;
   simd_array_ptr<float> bbox_min_y;
   simd_array_ptr<float> bbox_min_z;

   simd_array_ptr<float> bbox_max_x;
   simd_array_ptr<float> bbox_max_y;
   simd_array_ptr<float> bbox_max_z;

   std::unique_ptr<std::array<glm::vec4, 3>[]> transforms;

   std::uint32_t active_count = 0;
   std::unique_ptr<std::uint32_t[]> active_instances;

   UINT constants_index = 0;

   D3D11_PRIMITIVE_TOPOLOGY topology;
   UINT index_count;
   UINT start_index;
   INT base_vertex;
};

struct Instance_list_textured {
   std::uint32_t instance_count = 0;

   simd_array_ptr<float> bbox_min_x;
   simd_array_ptr<float> bbox_min_y;
   simd_array_ptr<float> bbox_min_z;

   simd_array_ptr<float> bbox_max_x;
   simd_array_ptr<float> bbox_max_y;
   simd_array_ptr<float> bbox_max_z;

   std::unique_ptr<std::array<glm::vec4, 3>[]> transforms;

   std::uint32_t active_count = 0;
   std::unique_ptr<std::uint32_t[]> active_instances;

   UINT constants_index = 0;

   D3D11_PRIMITIVE_TOPOLOGY topology;
   UINT index_count;
   UINT start_index;
   INT base_vertex;

   std::uint32_t texture_index = 0;
};

struct Instance_gpu_constants {
   glm::vec3 position_decompress_mul;
   std::uint32_t pad0;

   glm::vec3 position_decompress_add;
   std::uint32_t pad1;

   std::array<float, 56> pad3;
};

static_assert(sizeof(Instance_gpu_constants) == 256);

struct Active_world_metrics {
   std::uint32_t used_cpu_memory = 0;
   std::uint32_t used_gpu_memory = 0;
};

struct Active_world {
   Com_ptr<ID3D11Buffer> constants_buffer;
   Com_ptr<ID3D11Buffer> instances_buffer;

   std::vector<Instance_list> opaque_instance_lists;
   std::vector<Instance_list> doublesided_instance_lists;
   std::vector<Instance_list_textured> hardedged_instance_lists;
   std::vector<Instance_list_textured> hardedged_doublesided_instance_lists;

   std::uint32_t active_opaque_instance_lists_count = 0;
   std::unique_ptr<std::uint32_t[]> active_opaque_instance_lists;

   std::uint32_t active_doublesided_instance_lists_count = 0;
   std::unique_ptr<std::uint32_t[]> active_doublesided_instance_lists;

   std::uint32_t active_hardedged_instance_lists_count = 0;
   std::unique_ptr<std::uint32_t[]> active_hardedged_instance_lists;

   std::uint32_t active_hardedged_doublesided_instance_lists_count = 0;
   std::unique_ptr<std::uint32_t[]> active_hardedged_doublesided_instance_lists;

   Active_world_metrics metrics;

   void clear();

   void build(ID3D11Device2& device, std::span<const Model> models,
              std::span<const Game_model> game_models,
              std::span<const Object_instance> object_instances) noexcept;
};

};