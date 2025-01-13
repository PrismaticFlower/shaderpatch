#include "active_world.hpp"

#include "../../logger.hpp"

#include <absl/container/flat_hash_map.h>

namespace sp::shadows {

namespace {

template<typename T>
auto make_aligned_ptr(const std::size_t count) -> simd_array_ptr<T>
{
   return simd_array_ptr<T>{
      static_cast<T*>(_aligned_malloc(sizeof(T) * count, alignof(__m128)))};
}

struct Instance_list_build {
   std::vector<std::uint32_t> object_list;

   UINT constants_index = 0;

   D3D11_PRIMITIVE_TOPOLOGY topology;
   UINT index_count;
   UINT start_index;
   INT base_vertex;
};

struct Instance_list_build_textured {
   std::vector<std::uint32_t> object_list;

   UINT constants_index = 0;

   D3D11_PRIMITIVE_TOPOLOGY topology;
   UINT index_count;
   UINT start_index;
   INT base_vertex;

   std::uint32_t texture_index = 0;
};

struct Active_world_build {
   std::vector<Instance_list_build> opaque_instance_lists;
   std::vector<Instance_list_build> doublesided_instance_lists;
   std::vector<Instance_list_build_textured> hardedged_instance_lists;
   std::vector<Instance_list_build_textured> hardedged_doublesided_instance_lists;
};

void build_world_bbox(const Object_instance& object, const Model& model,
                      float& out_min_x, float& out_min_y, float& out_min_z,
                      float& out_max_x, float& out_max_y, float& out_max_z) noexcept
{
   (void)object, model;

   out_min_x = out_min_y = out_min_z = out_max_x = out_max_y = out_max_z = 0.0f;
}

auto count_memory_usage(const Active_world& world) noexcept -> std::size_t
{
   std::size_t count = 0;

   count += world.opaque_instance_lists.size() * sizeof(Instance_list);
   count += world.opaque_instance_lists.size() * sizeof(std::uint32_t);

   count += world.doublesided_instance_lists.size() * sizeof(Instance_list);
   count += world.doublesided_instance_lists.size() * sizeof(std::uint32_t);

   count += world.hardedged_instance_lists.size() * sizeof(Instance_list_textured);
   count += world.hardedged_instance_lists.size() * sizeof(std::uint32_t);

   count += world.hardedged_doublesided_instance_lists.size() *
            sizeof(Instance_list_textured);
   count += world.hardedged_doublesided_instance_lists.size() * sizeof(std::uint32_t);

   for (const Instance_list& list : world.opaque_instance_lists) {
      count += list.instance_count *
               (sizeof(float[6]) + sizeof(std::array<glm::vec4, 3>) +
                sizeof(std::uint32_t));
   }

   for (const Instance_list& list : world.doublesided_instance_lists) {
      count += list.instance_count *
               (sizeof(float[6]) + sizeof(std::array<glm::vec4, 3>) +
                sizeof(std::uint32_t));
   }

   for (const Instance_list_textured& list : world.hardedged_instance_lists) {
      count += list.instance_count *
               (sizeof(float[6]) + sizeof(std::array<glm::vec4, 3>) +
                sizeof(std::uint32_t));
   }

   for (const Instance_list_textured& list : world.hardedged_doublesided_instance_lists) {
      count += list.instance_count *
               (sizeof(float[6]) + sizeof(std::array<glm::vec4, 3>) +
                sizeof(std::uint32_t));
   }

   return count;
}

}

void Aligned_delete::operator()(void* ptr) const noexcept
{
   _aligned_free(ptr);
}

void Active_world::clear()
{
   *this = {};
}

void Active_world::build(ID3D11Device2& device, std::span<const Model> models,
                         std::span<const Game_model> game_models,
                         std::span<const Object_instance> object_instances) noexcept
{
   clear();

   std::vector<std::uint32_t> gpu_constants_index;
   gpu_constants_index.reserve(object_instances.size());

   struct Instance_list_ref {
      std::uint32_t opaque_start = 0;
      std::uint32_t doublesided_start = 0;
      std::uint32_t hardedged_start = 0;
      std::uint32_t hardedged_doublesided_start = 0;
   };

   absl::flat_hash_map<std::uint32_t, Instance_list_ref> instance_list_index;

   Active_world_build build;

   for (std::uint32_t object_index = 0; object_index < object_instances.size();
        ++object_index) {
      const Object_instance& object = object_instances[object_index];
      const Game_model& game_model = game_models[object.game_model_index];

      const auto existing = instance_list_index.find(game_model.lod0_index);

      const Instance_list_ref ref =
         existing == instance_list_index.end()
            ? Instance_list_ref{.opaque_start = build.opaque_instance_lists.size(),
                                .doublesided_start =
                                   build.doublesided_instance_lists.size(),
                                .hardedged_start =
                                   build.hardedged_instance_lists.size(),
                                .hardedged_doublesided_start =
                                   build.hardedged_doublesided_instance_lists.size()}
            : existing->second;

      const Model& model = models[game_model.lod0_index];

      if (existing == instance_list_index.end()) {
         const std::size_t constants_index = gpu_constants_index.size();

         gpu_constants_index.push_back(game_model.lod0_index);
         instance_list_index.emplace(game_model.lod0_index, ref);

         for (const Model_segment& segment : model.opaque_segments) {
            build.opaque_instance_lists.push_back({
               .constants_index = constants_index,

               .topology = segment.topology,
               .index_count = segment.index_count,
               .start_index = segment.start_index,
               .base_vertex = segment.base_vertex,
            });
         }

         for (const Model_segment& segment : model.doublesided_segments) {
            build.doublesided_instance_lists.push_back({
               .constants_index = constants_index,

               .topology = segment.topology,
               .index_count = segment.index_count,
               .start_index = segment.start_index,
               .base_vertex = segment.base_vertex,
            });
         }

         for (const Model_segment_hardedged& segment : model.hardedged_segments) {
            build.hardedged_instance_lists.push_back({
               .constants_index = constants_index,

               .topology = segment.topology,
               .index_count = segment.index_count,
               .start_index = segment.start_index,
               .base_vertex = segment.base_vertex,
            });
         }

         for (const Model_segment_hardedged& segment :
              model.hardedged_doublesided_segments) {
            build.hardedged_doublesided_instance_lists.push_back({
               .constants_index = constants_index,

               .topology = segment.topology,
               .index_count = segment.index_count,
               .start_index = segment.start_index,
               .base_vertex = segment.base_vertex,
            });
         }
      }

      for (std::uint32_t i = 0; i < model.opaque_segments.size(); ++i) {
         build.opaque_instance_lists[ref.opaque_start + i].object_list.push_back(
            object_index);
      }

      for (std::uint32_t i = 0; i < model.doublesided_segments.size(); ++i) {
         build.doublesided_instance_lists[ref.doublesided_start + i].object_list.push_back(
            object_index);
      }

      for (std::uint32_t i = 0; i < model.hardedged_segments.size(); ++i) {
         build.hardedged_instance_lists[ref.hardedged_start + i].object_list.push_back(
            object_index);
      }

      for (std::uint32_t i = 0; i < model.hardedged_doublesided_segments.size(); ++i) {
         build
            .hardedged_doublesided_instance_lists[ref.hardedged_doublesided_start + i]
            .object_list.push_back(object_index);
      }
   }

   opaque_instance_lists.reserve(build.opaque_instance_lists.size());
   doublesided_instance_lists.reserve(build.doublesided_instance_lists.size());
   hardedged_instance_lists.reserve(build.hardedged_instance_lists.size());
   hardedged_doublesided_instance_lists.reserve(
      build.hardedged_doublesided_instance_lists.size());

   for (const Instance_list_build& build_list : build.opaque_instance_lists) {
      const std::size_t object_count = build_list.object_list.size();

      opaque_instance_lists.push_back({
         .instance_count = object_count,

         .bbox_min_x = make_aligned_ptr<float>(object_count),
         .bbox_min_y = make_aligned_ptr<float>(object_count),
         .bbox_min_z = make_aligned_ptr<float>(object_count),

         .bbox_max_x = make_aligned_ptr<float>(object_count),
         .bbox_max_y = make_aligned_ptr<float>(object_count),
         .bbox_max_z = make_aligned_ptr<float>(object_count),

         .transforms = std::make_unique_for_overwrite<std::array<glm::vec4, 3>[]>(object_count),

         .active_instances = std::make_unique_for_overwrite<std::uint32_t[]>(object_count),

         .constants_index = build_list.constants_index,

         .topology = build_list.topology,
         .index_count = build_list.index_count,
         .start_index = build_list.start_index,
         .base_vertex = build_list.base_vertex,
      });

      Instance_list& list = opaque_instance_lists.back();

      for (std::uint32_t i = 0; i < build_list.object_list.size(); ++i) {
         const Object_instance& object = object_instances[build_list.object_list[i]];

         list.transforms[i] = {glm::vec4{object.rotation[0], object.positionWS.x},
                               glm::vec4{object.rotation[1], object.positionWS.y},
                               glm::vec4{object.rotation[2], object.positionWS.z}};

         build_world_bbox(object,
                          models[game_models[object.game_model_index].lod0_index],
                          list.bbox_min_x.get()[i], list.bbox_min_y.get()[i],
                          list.bbox_min_z.get()[i], list.bbox_max_x.get()[i],
                          list.bbox_max_y.get()[i], list.bbox_max_z.get()[i]);
      }
   }

   for (const Instance_list_build& build_list : build.doublesided_instance_lists) {
      const std::size_t object_count = build_list.object_list.size();

      doublesided_instance_lists.push_back({
         .instance_count = object_count,

         .bbox_min_x = make_aligned_ptr<float>(object_count),
         .bbox_min_y = make_aligned_ptr<float>(object_count),
         .bbox_min_z = make_aligned_ptr<float>(object_count),

         .bbox_max_x = make_aligned_ptr<float>(object_count),
         .bbox_max_y = make_aligned_ptr<float>(object_count),
         .bbox_max_z = make_aligned_ptr<float>(object_count),

         .transforms = std::make_unique_for_overwrite<std::array<glm::vec4, 3>[]>(object_count),

         .active_instances = std::make_unique_for_overwrite<std::uint32_t[]>(object_count),

         .constants_index = build_list.constants_index,

         .topology = build_list.topology,
         .index_count = build_list.index_count,
         .start_index = build_list.start_index,
         .base_vertex = build_list.base_vertex,
      });

      Instance_list& list = doublesided_instance_lists.back();

      for (std::uint32_t i = 0; i < build_list.object_list.size(); ++i) {
         const Object_instance& object = object_instances[build_list.object_list[i]];

         list.transforms[i] = {glm::vec4{object.rotation[0], object.positionWS.x},
                               glm::vec4{object.rotation[1], object.positionWS.y},
                               glm::vec4{object.rotation[2], object.positionWS.z}};

         build_world_bbox(object,
                          models[game_models[object.game_model_index].lod0_index],
                          list.bbox_min_x.get()[i], list.bbox_min_y.get()[i],
                          list.bbox_min_z.get()[i], list.bbox_max_x.get()[i],
                          list.bbox_max_y.get()[i], list.bbox_max_z.get()[i]);
      }
   }

   for (const Instance_list_build_textured& build_list : build.hardedged_instance_lists) {
      const std::size_t object_count = build_list.object_list.size();

      hardedged_instance_lists.push_back(Instance_list_textured{
         .instance_count = object_count,

         .bbox_min_x = make_aligned_ptr<float>(object_count),
         .bbox_min_y = make_aligned_ptr<float>(object_count),
         .bbox_min_z = make_aligned_ptr<float>(object_count),

         .bbox_max_x = make_aligned_ptr<float>(object_count),
         .bbox_max_y = make_aligned_ptr<float>(object_count),
         .bbox_max_z = make_aligned_ptr<float>(object_count),

         .transforms = std::make_unique_for_overwrite<std::array<glm::vec4, 3>[]>(object_count),

         .active_instances = std::make_unique_for_overwrite<std::uint32_t[]>(object_count),

         .constants_index = build_list.constants_index,

         .topology = build_list.topology,
         .index_count = build_list.index_count,
         .start_index = build_list.start_index,
         .base_vertex = build_list.base_vertex,

         .texture_index = build_list.texture_index,
      });

      Instance_list_textured& list = hardedged_instance_lists.back();

      for (std::uint32_t i = 0; i < build_list.object_list.size(); ++i) {
         const Object_instance& object = object_instances[build_list.object_list[i]];

         list.transforms[i] = {glm::vec4{object.rotation[0], object.positionWS.x},
                               glm::vec4{object.rotation[1], object.positionWS.y},
                               glm::vec4{object.rotation[2], object.positionWS.z}};

         build_world_bbox(object,
                          models[game_models[object.game_model_index].lod0_index],
                          list.bbox_min_x.get()[i], list.bbox_min_y.get()[i],
                          list.bbox_min_z.get()[i], list.bbox_max_x.get()[i],
                          list.bbox_max_y.get()[i], list.bbox_max_z.get()[i]);
      }
   }

   for (const Instance_list_build_textured& build_list :
        build.hardedged_doublesided_instance_lists) {
      const std::size_t object_count = build_list.object_list.size();

      hardedged_doublesided_instance_lists.push_back(Instance_list_textured{
         .instance_count = object_count,

         .bbox_min_x = make_aligned_ptr<float>(object_count),
         .bbox_min_y = make_aligned_ptr<float>(object_count),
         .bbox_min_z = make_aligned_ptr<float>(object_count),

         .bbox_max_x = make_aligned_ptr<float>(object_count),
         .bbox_max_y = make_aligned_ptr<float>(object_count),
         .bbox_max_z = make_aligned_ptr<float>(object_count),

         .transforms = std::make_unique_for_overwrite<std::array<glm::vec4, 3>[]>(object_count),

         .active_instances = std::make_unique_for_overwrite<std::uint32_t[]>(object_count),

         .constants_index = build_list.constants_index,

         .topology = build_list.topology,
         .index_count = build_list.index_count,
         .start_index = build_list.start_index,
         .base_vertex = build_list.base_vertex,

         .texture_index = build_list.texture_index,
      });

      Instance_list_textured& list = hardedged_doublesided_instance_lists.back();

      for (std::uint32_t i = 0; i < build_list.object_list.size(); ++i) {
         const Object_instance& object = object_instances[build_list.object_list[i]];

         list.transforms[i] = {glm::vec4{object.rotation[0], object.positionWS.x},
                               glm::vec4{object.rotation[1], object.positionWS.y},
                               glm::vec4{object.rotation[2], object.positionWS.z}};

         build_world_bbox(object,
                          models[game_models[object.game_model_index].lod0_index],
                          list.bbox_min_x.get()[i], list.bbox_min_y.get()[i],
                          list.bbox_min_z.get()[i], list.bbox_max_x.get()[i],
                          list.bbox_max_y.get()[i], list.bbox_max_z.get()[i]);
      }
   }

   active_opaque_instance_lists = std::make_unique_for_overwrite<std::uint32_t[]>(
      build.opaque_instance_lists.size());

   active_doublesided_instance_lists =
      std::make_unique_for_overwrite<std::uint32_t[]>(
         build.doublesided_instance_lists.size());

   active_hardedged_instance_lists = std::make_unique_for_overwrite<std::uint32_t[]>(
      build.hardedged_instance_lists.size());

   active_hardedged_doublesided_instance_lists =
      std::make_unique_for_overwrite<std::uint32_t[]>(
         build.hardedged_doublesided_instance_lists.size());

   std::uint32_t total_instance_count = 0;

   for (const Instance_list& list : opaque_instance_lists) {
      total_instance_count += list.instance_count;
   }

   for (const Instance_list& list : doublesided_instance_lists) {
      total_instance_count += list.instance_count;
   }

   for (const Instance_list_textured& list : hardedged_instance_lists) {
      total_instance_count += list.instance_count;
   }

   for (const Instance_list_textured& list : hardedged_doublesided_instance_lists) {
      total_instance_count += list.instance_count;
   }
   const D3D11_BUFFER_DESC instances_buffer_desc{
      .ByteWidth = total_instance_count * sizeof(std::array<glm::vec4, 3>),
      .Usage = D3D11_USAGE_DYNAMIC,
      .BindFlags = D3D11_BIND_VERTEX_BUFFER,
      .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
   };

   if (FAILED(device.CreateBuffer(&instances_buffer_desc, nullptr,
                                  instances_buffer.clear_and_assign()))) {
      log_and_terminate_fmt(
         "Failed to shadow world create instance buffer! Buffer Size: {}",
         instances_buffer_desc.ByteWidth);
   }

   build = {};

   std::vector<Instance_gpu_constants> gpu_constants;
   gpu_constants.reserve(gpu_constants_index.size());

   for (const std::uint32_t model_index : gpu_constants_index) {
      const Model& model = models[model_index];

      gpu_constants.push_back({
         .position_decompress_mul = (model.bbox_max - model.bbox_min) * (0.5f / INT16_MAX),
         .position_decompress_add = (model.bbox_max + model.bbox_min) * 0.5f,
      });
   }

   const D3D11_BUFFER_DESC constants_buffer_desc{
      .ByteWidth = gpu_constants.size() * sizeof(Instance_gpu_constants),
      .Usage = D3D11_USAGE_IMMUTABLE,
      .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
   };

   const D3D11_SUBRESOURCE_DATA initial_data{.pSysMem = gpu_constants.data()};

   if (FAILED(device.CreateBuffer(&constants_buffer_desc, &initial_data,
                                  constants_buffer.clear_and_assign()))) {
      log_and_terminate_fmt(
         "Failed to shadow world create constants buffer! Buffer Size: {}",
         constants_buffer_desc.ByteWidth);
   }

   metrics.used_cpu_memory = count_memory_usage(*this);
   metrics.used_gpu_memory =
      instances_buffer_desc.ByteWidth + constants_buffer_desc.ByteWidth;
}

}