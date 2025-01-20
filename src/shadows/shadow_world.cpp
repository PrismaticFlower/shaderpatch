#include "shadow_world.hpp"

#include "frustum.hpp"

#include "internal/active_world.hpp"
#include "internal/debug_model_draw.hpp"
#include "internal/debug_world_aabb_draw.hpp"
#include "internal/debug_world_draw.hpp"
#include "internal/debug_world_textured_draw.hpp"
#include "internal/draw_resources.hpp"
#include "internal/mesh_buffer.hpp"
#include "internal/mesh_copy_queue.hpp"
#include "internal/name_table.hpp"
#include "internal/texture_table.hpp"
#include "internal/world/entity_class.hpp"
#include "internal/world/game_model.hpp"
#include "internal/world/model.hpp"
#include "internal/world/object_instance.hpp"

#include "../imgui/imgui.h"
#include "../logger.hpp"

#include "com_ptr.hpp"
#include "swbf_fnv_1a.hpp"

#include <d3d11_2.h>

#include <shared_mutex>

namespace sp::shadows {

namespace {

const UINT MESH_BUFFER_SIZE = 0x4000000;
const UINT MAX_SEGMENT_VERTICES = D3D11_16BIT_INDEX_STRIP_CUT_VALUE - 1;

struct Shadow_world {
   Shadow_world(ID3D11Device2& device, shader::Database& shaders)
      : _device{copy_raw_com_ptr(device)}, _draw_resources{device, shaders}
   {
   }

   void process_mesh_copy_queue(ID3D11DeviceContext2& dc) noexcept
   {
      std::scoped_lock lock{_mutex};

      _mesh_copy_queue.process(dc);
   }

   void draw_shadow_views(ID3D11DeviceContext2& dc, const glm::mat4& view_proj_matrix,
                          std::span<const Shadow_draw_view> views) noexcept
   {
      std::scoped_lock lock{_mutex};

      if (_active_rebuild_needed) build_active_world();

      dc.ClearState();

      dc.OMSetDepthStencilState(_draw_resources.depth_stencil_state.get(), 0xff);

      dc.PSSetSamplers(0, 1, _draw_resources.sampler_state.get_ptr());

      Frustum view_frustum{glm::inverse(glm::dmat4{view_proj_matrix})};

      for (const Shadow_draw_view& view : views) {
         Frustum shadow_frustum{glm::inverse(glm::dmat4{view.shadow_projection_matrix})};

         for (std::uint32_t list_index = 0;
              list_index < _active_world.opaque_instance_lists.size(); ++list_index) {
            Instance_list& list = _active_world.opaque_instance_lists[list_index];

            for (std::uint32_t instance_index = 0;
                 instance_index < list.instance_count; ++instance_index) {
               const Bounding_box bbox = {
                  .min = {list.bbox_min_x.get()[instance_index],
                          list.bbox_min_y.get()[instance_index],
                          list.bbox_min_z.get()[instance_index]},
                  .max = {list.bbox_max_x.get()[instance_index],
                          list.bbox_max_y.get()[instance_index],
                          list.bbox_max_z.get()[instance_index]},
               };

               // if (intersects(view_frustum, bbox)) continue;
               if (!intersects(shadow_frustum, bbox)) continue;

               list.active_instances[list.active_count] = instance_index;

               list.active_count += 1;
            }

            if (list.active_count != 0) {
               _active_world.active_opaque_instance_lists[_active_world.active_opaque_instance_lists_count] =
                  list_index;
               _active_world.active_opaque_instance_lists_count += 1;
            }
         }

         for (std::uint32_t list_index = 0;
              list_index < _active_world.doublesided_instance_lists.size();
              ++list_index) {
            Instance_list& list = _active_world.doublesided_instance_lists[list_index];

            for (std::uint32_t instance_index = 0;
                 instance_index < list.instance_count; ++instance_index) {
               const Bounding_box bbox = {
                  .min = {list.bbox_min_x.get()[instance_index],
                          list.bbox_min_y.get()[instance_index],
                          list.bbox_min_z.get()[instance_index]},
                  .max = {list.bbox_max_x.get()[instance_index],
                          list.bbox_max_y.get()[instance_index],
                          list.bbox_max_z.get()[instance_index]},
               };

               //  if (intersects(view_frustum, bbox)) continue;
               if (!intersects(shadow_frustum, bbox)) continue;

               list.active_instances[list.active_count] = instance_index;

               list.active_count += 1;
            }

            if (list.active_count != 0) {
               _active_world.active_doublesided_instance_lists[_active_world.active_doublesided_instance_lists_count] =
                  list_index;
               _active_world.active_doublesided_instance_lists_count += 1;
            }
         }

         for (std::uint32_t list_index = 0;
              list_index < _active_world.hardedged_instance_lists.size();
              ++list_index) {
            Instance_list_textured& list =
               _active_world.hardedged_instance_lists[list_index];

            for (std::uint32_t instance_index = 0;
                 instance_index < list.instance_count; ++instance_index) {
               const Bounding_box bbox = {
                  .min = {list.bbox_min_x.get()[instance_index],
                          list.bbox_min_y.get()[instance_index],
                          list.bbox_min_z.get()[instance_index]},
                  .max = {list.bbox_max_x.get()[instance_index],
                          list.bbox_max_y.get()[instance_index],
                          list.bbox_max_z.get()[instance_index]},
               };

               //   if (intersects(view_frustum, bbox)) continue;
               if (!intersects(shadow_frustum, bbox)) continue;

               list.active_instances[list.active_count] = instance_index;

               list.active_count += 1;
            }

            if (list.active_count != 0) {
               _active_world.active_hardedged_instance_lists[_active_world.active_hardedged_instance_lists_count] =
                  list_index;
               _active_world.active_hardedged_instance_lists_count += 1;
            }
         }

         for (std::uint32_t list_index = 0;
              list_index < _active_world.hardedged_doublesided_instance_lists.size();
              ++list_index) {
            Instance_list_textured& list =
               _active_world.hardedged_doublesided_instance_lists[list_index];

            for (std::uint32_t instance_index = 0;
                 instance_index < list.instance_count; ++instance_index) {
               const Bounding_box bbox = {
                  .min = {list.bbox_min_x.get()[instance_index],
                          list.bbox_min_y.get()[instance_index],
                          list.bbox_min_z.get()[instance_index]},
                  .max = {list.bbox_max_x.get()[instance_index],
                          list.bbox_max_y.get()[instance_index],
                          list.bbox_max_z.get()[instance_index]},
               };

               //  if (intersects(view_frustum, bbox)) continue;
               if (!intersects(shadow_frustum, bbox)) continue;

               list.active_instances[list.active_count] = instance_index;

               list.active_count += 1;
            }

            if (list.active_count != 0) {
               _active_world
                  .active_hardedged_doublesided_instance_lists[_active_world.active_hardedged_doublesided_instance_lists_count] =
                  list_index;
               _active_world.active_hardedged_doublesided_instance_lists_count += 1;
            }
         }

         _counter_total_draws = 0;
         _counter_total_instances = 0;

         D3D11_MAPPED_SUBRESOURCE mapped_view_constants{};

         if (FAILED(dc.Map(_draw_resources.view_constants.get(), 0,
                           D3D11_MAP_WRITE_DISCARD, 0, &mapped_view_constants))) {
            log_and_terminate("Mapping shadow world view constants failed!");
         }

         std::memcpy(mapped_view_constants.pData, &view.shadow_projection_matrix,
                     sizeof(view.shadow_projection_matrix));

         dc.Unmap(_draw_resources.view_constants.get(), 0);

         dc.RSSetViewports(1, &view.viewport);

         dc.OMSetRenderTargets(0, nullptr, view.dsv);

         _active_world.upload_instance_buffer(dc);

         dc.IASetInputLayout(_draw_resources.input_layout.get());
         dc.IASetIndexBuffer(_index_buffer.get(), DXGI_FORMAT_R16_UINT, 0);

         ID3D11Buffer* vertex_buffer = _vertex_buffer.get();
         const UINT vertex_buffer_stride = 6;

         const UINT instance_buffer_stride = sizeof(std::array<glm::vec4, 3>);
         const UINT vertex_buffer_offset = 0;

         dc.IASetVertexBuffers(
            0, 2,
            std::array<ID3D11Buffer*, 2>{vertex_buffer,
                                         _active_world.instances_buffer.get()}
               .data(),
            std::array<UINT, 2>{vertex_buffer_stride, instance_buffer_stride}.data(),
            std::array<UINT, 2>{vertex_buffer_offset, vertex_buffer_offset}.data());

         dc.VSSetConstantBuffers(1, 1, _draw_resources.view_constants.get_ptr());
         dc.VSSetShader(_draw_resources.vertex_shader.get(), nullptr, 0);

         dc.RSSetState(_draw_resources.rasterizer_state.get());

         dc.PSSetShader(nullptr, nullptr, 0);

         const UINT constants_count = 16;

         UINT start_instance = 0;
         D3D11_PRIMITIVE_TOPOLOGY current_topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;

         for (std::uint32_t list_index :
              std::span{_active_world.active_opaque_instance_lists.get(),
                        _active_world.active_opaque_instance_lists_count}) {
            Instance_list& list = _active_world.opaque_instance_lists[list_index];

            if (current_topology != list.topology) {
               dc.IASetPrimitiveTopology(list.topology);

               current_topology = list.topology;
            }

            const UINT first_constant = list.constants_index * (256 / 16);

            dc.VSSetConstantBuffers1(0, 1, _active_world.constants_buffer.get_ptr(),
                                     &first_constant, &constants_count);

            dc.DrawIndexedInstanced(list.index_count, list.active_count, list.start_index,
                                    list.base_vertex, start_instance);

            start_instance += list.active_count;
         }

         dc.RSSetState(_draw_resources.rasterizer_state_doublesided.get());

         for (std::uint32_t list_index :
              std::span{_active_world.active_doublesided_instance_lists.get(),
                        _active_world.active_doublesided_instance_lists_count}) {
            Instance_list& list = _active_world.doublesided_instance_lists[list_index];

            if (current_topology != list.topology) {
               dc.IASetPrimitiveTopology(list.topology);

               current_topology = list.topology;
            }

            const UINT first_constant = list.constants_index * (256 / 16);

            dc.VSSetConstantBuffers1(0, 1, _active_world.constants_buffer.get_ptr(),
                                     &first_constant, &constants_count);

            dc.DrawIndexedInstanced(list.index_count, list.active_count, list.start_index,
                                    list.base_vertex, start_instance);

            start_instance += list.active_count;
         }

         const UINT vertex_buffer_textured_stride = 10;

         dc.IASetInputLayout(_draw_resources.input_layout_textured.get());
         dc.IASetVertexBuffers(0, 1, &vertex_buffer, &vertex_buffer_textured_stride,
                               &vertex_buffer_offset);

         dc.VSSetShader(_draw_resources.vertex_shader_textured.get(), nullptr, 0);
         dc.PSSetShader(_draw_resources.pixel_shader_hardedged.get(), nullptr, 0);

         for (std::uint32_t list_index : std::span{
                 _active_world.active_hardedged_doublesided_instance_lists.get(),
                 _active_world.active_hardedged_doublesided_instance_lists_count}) {
            Instance_list_textured& list =
               _active_world.hardedged_doublesided_instance_lists[list_index];

            if (current_topology != list.topology) {
               dc.IASetPrimitiveTopology(list.topology);

               current_topology = list.topology;
            }

            const UINT first_constant = list.constants_index * (256 / 16);

            dc.VSSetConstantBuffers1(0, 1, _active_world.constants_buffer.get_ptr(),
                                     &first_constant, &constants_count);

            dc.PSSetShaderResources(0, 1, _texture_table.get_ptr(list.texture_index));

            dc.DrawIndexedInstanced(list.index_count, list.active_count, list.start_index,
                                    list.base_vertex, start_instance);

            start_instance += list.active_count;
         }

         dc.RSSetState(_draw_resources.rasterizer_state.get());

         for (std::uint32_t list_index :
              std::span{_active_world.active_hardedged_instance_lists.get(),
                        _active_world.active_hardedged_instance_lists_count}) {
            Instance_list_textured& list =
               _active_world.hardedged_instance_lists[list_index];

            if (current_topology != list.topology) {
               dc.IASetPrimitiveTopology(list.topology);

               current_topology = list.topology;
            }

            const UINT first_constant = list.constants_index * (256 / 16);

            dc.VSSetConstantBuffers1(0, 1, _active_world.constants_buffer.get_ptr(),
                                     &first_constant, &constants_count);

            dc.PSSetShaderResources(0, 1, _texture_table.get_ptr(list.texture_index));

            dc.DrawIndexedInstanced(list.index_count, list.active_count, list.start_index,
                                    list.base_vertex, start_instance);

            start_instance += list.active_count;
         }

         _counter_total_draws =
            _active_world.active_opaque_instance_lists_count +
            _active_world.active_doublesided_instance_lists_count +
            _active_world.active_hardedged_instance_lists_count +
            _active_world.active_hardedged_doublesided_instance_lists_count;
         _counter_total_instances = start_instance;

         _active_world.reset();
      }
   }

   void draw_shadow_world_preview(ID3D11DeviceContext2& dc,
                                  const glm::mat4& projection_matrix,
                                  const D3D11_VIEWPORT& viewport,
                                  ID3D11RenderTargetView* rtv,
                                  ID3D11DepthStencilView* dsv) noexcept
   {
      std::scoped_lock lock{_mutex};

      _debug_world_draw.draw(dc,
                             {
                                .models = _models,
                                .game_models = _game_models,
                                .object_instances = _object_instances,

                             },
                             projection_matrix, _index_buffer.get(),
                             _vertex_buffer.get(),
                             {
                                .viewport = viewport,
                                .rtv = rtv,
                                .picking_rtv = nullptr,
                                .dsv = dsv,
                             });
   }

   void draw_shadow_world_textured_preview(ID3D11DeviceContext2& dc,
                                           const glm::mat4& projection_matrix,
                                           const D3D11_VIEWPORT& viewport,
                                           ID3D11RenderTargetView* rtv,
                                           ID3D11DepthStencilView* dsv) noexcept
   {
      std::scoped_lock lock{_mutex};

      _debug_world_textured_draw.draw(dc,
                                      {
                                         .models = _models,
                                         .game_models = _game_models,
                                         .object_instances = _object_instances,
                                         .texture_table = _texture_table,

                                      },
                                      projection_matrix, _index_buffer.get(),
                                      _vertex_buffer.get(),
                                      {
                                         .viewport = viewport,
                                         .rtv = rtv,
                                         .dsv = dsv,
                                      });
   }

   void draw_shadow_world_aabb_overlay(ID3D11DeviceContext2& dc,
                                       const glm::mat4& projection_matrix,
                                       const D3D11_VIEWPORT& viewport,
                                       ID3D11RenderTargetView* rtv,
                                       ID3D11DepthStencilView* dsv) noexcept
   {
      std::scoped_lock lock{_mutex};

      _debug_world_aabb_draw.draw(dc,
                                  {
                                     .models = _models,
                                     .game_models = _game_models,
                                     .object_instances = _object_instances,
                                  },
                                  projection_matrix,
                                  {
                                     .viewport = viewport,
                                     .rtv = rtv,
                                     .dsv = dsv,
                                  });
   }

   void clear() noexcept
   {
      std::scoped_lock lock{_mutex};

      _name_table.clear();

      _index_buffer.clear();
      _vertex_buffer.clear();

      _mesh_copy_queue.clear();

      _models.clear();
      _models_index.clear();

      _game_models.clear();
      _game_models_index.clear();

      _entity_classes.clear();
      _entity_classes_index.clear();

      _object_instances.clear();

      _active_world.clear();

      _texture_table.clear();

      _active_rebuild_needed = false;
   }

   void add_texture(const Input_texture& input_texture) noexcept
   {
      std::scoped_lock lock{_mutex};

      log_debug("Read texture '{}' (hash: "
                "{:08x}{:08x}{:08x}{:08x}{:08x}{:08x}{:08x}{:08x})",
                input_texture.name, input_texture.hash.words[0],
                input_texture.hash.words[1], input_texture.hash.words[2],
                input_texture.hash.words[3], input_texture.hash.words[4],
                input_texture.hash.words[5], input_texture.hash.words[6],
                input_texture.hash.words[7]);

      _texture_table.add(_name_table.add(input_texture.name), input_texture.hash);
   }

   void add_model(const Input_model& input_model) noexcept
   {
      std::scoped_lock lock{_mutex};

      if (input_model.segments.empty()) {
         log_fmt(Log_level::info, "Discarding model '{}' with no usable segments for shadows. (It is either skinned or fully transparent).",
                 input_model.name);

         return;
      }

      log_debug("Read model '{}' (vert min: X {} Y {} Z {} vert max: X {} Y {} "
                "Z {} segments: {})",
                input_model.name, input_model.min_vertex.x,
                input_model.min_vertex.y, input_model.min_vertex.z,
                input_model.max_vertex.x, input_model.max_vertex.y,
                input_model.max_vertex.z, input_model.segments.size());

      const std::uint32_t name_hash = _name_table.add(input_model.name);

      if (_models_index.contains(name_hash)) {
         log_debug("Skipping adding model '{}' as a model with the same name "
                   "hash already exists.",
                   input_model.name);

         return;
      }

      Model_merge_segment opaque_segment;
      Model_merge_segment doublesided_segment;
      Model_merge_segment_hardedged hardedged_segment;
      Model_merge_segment_hardedged hardedged_doublesided_segment;

      Model model;

      for (const Input_model::Segment& segment : input_model.segments) {
         const D3D11_PRIMITIVE_TOPOLOGY topology = [&]() {
            switch (segment.topology) {
            case Input_model::Topology::tri_list:
               return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            case Input_model::Topology::tri_strip:
            default:
               return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
            }
         }();

         if (segment.hardedged) {
            Model_merge_segment_hardedged& merge_segment =
               segment.doublesided ? hardedged_doublesided_segment : hardedged_segment;

            if (merge_segment.vertices.empty()) {
               merge_segment.topology = topology;
               merge_segment.texture_name_hash =
                  _name_table.add(segment.diffuse_texture);
            }
            else if (merge_segment.vertices.size() + segment.vertices.size() >
                        MAX_SEGMENT_VERTICES ||
                     merge_segment.topology != topology ||
                     merge_segment.texture_name_hash !=
                        fnv_1a_hash(segment.diffuse_texture)) {
               add_model_segment(input_model.name, merge_segment,
                                 segment.doublesided ? model.hardedged_doublesided_segments
                                                     : model.hardedged_segments);

               merge_segment.indices.clear();
               merge_segment.vertices.clear();
               merge_segment.topology = topology;
               merge_segment.texture_name_hash =
                  _name_table.add(segment.diffuse_texture);
            }

            const std::size_t index_offset = merge_segment.vertices.size();

            merge_segment.vertices.reserve(merge_segment.vertices.size() +
                                           segment.vertices.size());

            for (std::size_t i = 0; i < segment.vertices.size(); ++i) {
               merge_segment.vertices.push_back(
                  {segment.vertices[i][0], segment.vertices[i][1],
                   segment.vertices[i][2], segment.texcoords[i][0],
                   segment.texcoords[i][1]});
            }

            const bool needs_cut = topology == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP &&
                                   !merge_segment.indices.empty();

            merge_segment.indices.reserve(
               merge_segment.indices.size() + segment.indices.size() + needs_cut ? 1 : 0);

            if (needs_cut) {
               merge_segment.indices.push_back(D3D11_16BIT_INDEX_STRIP_CUT_VALUE);
            }

            for (const std::uint16_t index : segment.indices) {
               merge_segment.indices.push_back(
                  static_cast<std::uint16_t>(index + index_offset));
            }
         }
         else {
            Model_merge_segment& merge_segment =
               segment.doublesided ? doublesided_segment : opaque_segment;

            if (merge_segment.vertices.empty()) {
               merge_segment.topology = topology;
            }
            else if (merge_segment.vertices.size() + segment.vertices.size() >
                        MAX_SEGMENT_VERTICES ||
                     merge_segment.topology != topology) {
               add_model_segment(input_model.name, merge_segment,
                                 segment.doublesided ? model.doublesided_segments
                                                     : model.opaque_segments);

               merge_segment.indices.clear();
               merge_segment.vertices.clear();
               merge_segment.topology = topology;
            }

            const std::size_t index_offset = merge_segment.vertices.size();

            merge_segment.vertices.insert(merge_segment.vertices.end(),
                                          segment.vertices.begin(),
                                          segment.vertices.end());

            const bool needs_cut = topology == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP &&
                                   !merge_segment.indices.empty();

            merge_segment.indices.reserve(
               merge_segment.indices.size() + segment.indices.size() + needs_cut ? 1 : 0);

            if (needs_cut) {
               merge_segment.indices.push_back(D3D11_16BIT_INDEX_STRIP_CUT_VALUE);
            }

            for (const std::uint16_t index : segment.indices) {
               merge_segment.indices.push_back(
                  static_cast<std::uint16_t>(index + index_offset));
            }
         }
      }

      if (!opaque_segment.vertices.empty()) {
         add_model_segment(input_model.name, opaque_segment, model.opaque_segments);
      }

      if (!doublesided_segment.vertices.empty()) {
         add_model_segment(input_model.name, doublesided_segment,
                           model.doublesided_segments);
      }

      if (!hardedged_segment.vertices.empty()) {
         add_model_segment(input_model.name, hardedged_segment, model.hardedged_segments);
      }

      if (!hardedged_doublesided_segment.vertices.empty()) {
         add_model_segment(input_model.name, hardedged_doublesided_segment,
                           model.hardedged_doublesided_segments);
      }

      model.position_decompress_mul =
         (input_model.max_vertex - input_model.min_vertex) * (0.5f / INT16_MAX);
      model.position_decompress_add =
         (input_model.max_vertex + input_model.min_vertex) * 0.5f;

      model.bbox = {.min = glm::vec3{FLT_MAX}, .max = glm::vec3{-FLT_MAX}};

      for (const Input_model::Segment& segment : input_model.segments) {
         for (const auto& [x, y, z] : segment.vertices) {
            const glm::vec3 positionCS = {x, y, z};
            const glm::vec3 positionOS = positionCS * model.position_decompress_mul +
                                         model.position_decompress_add;

            model.bbox.min = glm::min(model.bbox.min, positionOS);
            model.bbox.max = glm::max(model.bbox.max, positionOS);
         }
      }

      const std::size_t model_index = _models.size();

      _models_index.emplace(name_hash, model_index);
      _models.push_back(std::move(model));
   }

   void add_game_model(const Input_game_model& input_game_model) noexcept
   {
      std::scoped_lock lock{_mutex};

      log_debug("Read game model '{}' LOD0: '{}' ({}) LOD1: '{}' ({}) LOD2: "
                "'{}' ({}) LOWD: '{}' ({})",
                input_game_model.name,
                input_game_model.lod0.empty() ? "<missing>" : input_game_model.lod0,
                input_game_model.lod0_tris,
                input_game_model.lod1.empty() ? "<missing>" : input_game_model.lod1,
                input_game_model.lod1_tris,
                input_game_model.lod2.empty() ? "<missing>" : input_game_model.lod2,
                input_game_model.lod2_tris,
                input_game_model.lod2.empty() ? "<missing>" : input_game_model.lod2,
                input_game_model.lod2_tris,
                input_game_model.lowd.empty() ? "<missing>" : input_game_model.lowd,
                input_game_model.lowd_tris);

      auto lod0_it = _models_index.find(fnv_1a_hash(input_game_model.lod0));

      if (lod0_it == _models_index.end()) {
         log_fmt(Log_level::info, "Discarding game model '{}' with missing (possibly discarded) model '{}'.",
                 input_game_model.name, input_game_model.lod0);

         return;
      }

      const std::uint32_t name_hash = _name_table.add(input_game_model.name);

      if (_game_models_index.contains(name_hash)) {
         log_debug("Skipping adding game model '{}' as a game model with the "
                   "same name hash already exists.",
                   input_game_model.name);

         return;
      }

      Game_model game_model{
         .lod0_index = lod0_it->second,
         .lod1_index = lod0_it->second,
         .lod2_index = lod0_it->second,
         .lowd_index = lod0_it->second,
      };

      if (auto lod1_it = _models_index.find(fnv_1a_hash(input_game_model.lod1));
          lod1_it != _models_index.end()) {
         game_model.lod1_index = lod1_it->second;
         game_model.lod2_index = lod1_it->second;
         game_model.lowd_index = lod1_it->second;
      }

      if (auto lod2_it = _models_index.find(fnv_1a_hash(input_game_model.lod2));
          lod2_it != _models_index.end()) {
         game_model.lod2_index = lod2_it->second;
         game_model.lowd_index = lod2_it->second;
      }

      if (auto lowd_it = _models_index.find(fnv_1a_hash(input_game_model.lowd));
          lowd_it != _models_index.end()) {
         game_model.lowd_index = lowd_it->second;
      }

      const std::size_t model_index = _game_models.size();

      _game_models_index.emplace(name_hash, model_index);
      _game_models.push_back(std::move(game_model));
   }

   void add_entity_class(const Input_entity_class& input_entity_class) noexcept
   {
      std::scoped_lock lock{_mutex};

      log_debug("Read entity class '{}' BASE: '{}' GeometryName: '{}'",
                input_entity_class.type_name, input_entity_class.base_name,
                input_entity_class.geometry_name);

      auto game_model_it =
         _game_models_index.find(fnv_1a_hash(input_entity_class.geometry_name));

      if (game_model_it == _game_models_index.end()) {
         log_fmt(Log_level::info, "Discarding entity class '{}' with missing (possibly discarded) game model '{}'.",
                 input_entity_class.type_name, input_entity_class.geometry_name);

         return;
      }

      const std::uint32_t name_hash = _name_table.add(input_entity_class.type_name);

      if (_entity_classes_index.contains(name_hash)) {
         log_debug("Skipping adding entity class '{}' as an entity class with "
                   "the same name hash already exists.",
                   input_entity_class.type_name);

         return;
      }

      const std::size_t entity_class_index = _entity_classes.size();

      _entity_classes_index.emplace(name_hash, entity_class_index);
      _entity_classes.push_back({.game_model_index = game_model_it->second});
   }

   void add_object_instance(const Input_object_instance& input_object_instance) noexcept
   {
      std::scoped_lock lock{_mutex};

      std::string_view name = input_object_instance.name;

      if (name.empty()) name = "<unnamed>";

      log_debug("Read object instance '{}' (type name: '{}' layer: '{}')", name,
                input_object_instance.type_name, input_object_instance.layer_name);

      auto entity_class_it =
         _entity_classes_index.find(fnv_1a_hash(input_object_instance.type_name));

      if (entity_class_it == _entity_classes_index.end()) {
         log_fmt(Log_level::info, "Discarding onject instance '{}' with missing (possibly discarded) entity class '{}'.",
                 name, input_object_instance.type_name);

         return;
      }

      _object_instances.push_back({
         .name_hash = _name_table.add(name),
         .layer_name_hash = _name_table.add(input_object_instance.layer_name),
         .game_model_index = _entity_classes[entity_class_it->second].game_model_index,
         .from_child_lvl = input_object_instance.in_child_lvl,
         .rotation =
            {
               glm::vec3{input_object_instance.rotation[0].x,
                         input_object_instance.rotation[1].x,
                         input_object_instance.rotation[2].x},
               glm::vec3{input_object_instance.rotation[0].y,
                         input_object_instance.rotation[1].y,
                         input_object_instance.rotation[2].y},
               glm::vec3{input_object_instance.rotation[0].z,
                         input_object_instance.rotation[1].z,
                         input_object_instance.rotation[2].z},
            },
         .positionWS = input_object_instance.positionWS,
      });

      _active_rebuild_needed = true;
   }

   void register_texture(ID3D11ShaderResourceView& srv, const Texture_hash& data_hash) noexcept
   {
      std::scoped_lock lock{_mutex};

      _texture_table.register_(srv, data_hash);
   }

   void unregister_texture(ID3D11ShaderResourceView& srv) noexcept
   {
      std::scoped_lock lock{_mutex};

      _texture_table.unregister(srv);
   }

   void show_imgui(ID3D11DeviceContext2& dc) noexcept
   {
      if (ImGui::Begin("Shadow World")) {
         if (ImGui::BeginTabBar("Tabs")) {
            if (ImGui::BeginTabItem("Metrics")) {
               ImGui::Text("Index Buffer Usage: %.1f / %.1f MB",
                           _index_buffer.allocated_bytes() / 1'000'000.0,
                           MESH_BUFFER_SIZE / 1'000'000.0);

               ImGui::Text("Vertex Buffer Usage: %.1f / %.1f MB",
                           _vertex_buffer.allocated_bytes() / 1'000'000.0,
                           MESH_BUFFER_SIZE / 1'000'000.0);

               ImGui::Text("Models Count: %u", _models.size());

               std::size_t cpu_memory = sizeof(Shadow_world);

               const auto get_bytes_capcity = []<typename T>(const T& container) {
                  return container.capacity() * sizeof(T::value_type);
               };

               cpu_memory += get_bytes_capcity(_models);
               cpu_memory += get_bytes_capcity(_models_index);
               cpu_memory += get_bytes_capcity(_game_models);
               cpu_memory += get_bytes_capcity(_game_models_index);
               cpu_memory += get_bytes_capcity(_entity_classes);
               cpu_memory += get_bytes_capcity(_entity_classes_index);
               cpu_memory += get_bytes_capcity(_object_instances);
               cpu_memory += _active_world.metrics.used_cpu_memory;

               cpu_memory += _texture_table.allocated_bytes();
               cpu_memory += _name_table.allocated_bytes();

               ImGui::Text("Approximate CPU Memory Used: %.1f KB", cpu_memory / 1'000.0);

               ImGui::SeparatorText("Active World");

               ImGui::Text("Approximate CPU Memory Used: %.1f KB",
                           _active_world.metrics.used_cpu_memory / 1'000.0);

               ImGui::Text("Approximate GPU Memory Used: %.1f KB",
                           _active_world.metrics.used_gpu_memory / 1'000.0);

               ImGui::SeparatorText("Draws");

               ImGui::Text("Total Draw Calls %u", _counter_total_draws);
               ImGui::Text("Total Instance Count %u", _counter_total_instances);

               ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Models")) {
               static std::uint32_t selected_model_hash = 0;
               std::uint32_t selected_model_index = UINT32_MAX;

               if (ImGui::BeginChild("##list",
                                     {ImGui::GetContentRegionAvail().x * 0.4f, 0.0f},
                                     ImGuiChildFlags_ResizeX |
                                        ImGuiChildFlags_FrameStyle)) {
                  for (const auto& [model_name_hash, model_index] : _models_index) {
                     const bool selected = selected_model_hash == model_name_hash;

                     const char* name = _name_table.lookup(model_name_hash);

                     if (ImGui::Selectable(name, selected)) {
                        selected_model_hash = model_name_hash;
                     }

                     ImGui::SetItemTooltip(name);

                     if (selected) selected_model_index = model_index;
                  }
               }

               ImGui::EndChild();

               ImGui::SameLine();

               if (ImGui::BeginChild("##model") &&
                   selected_model_index < _models.size()) {
                  const Model& model = _models[selected_model_index];

                  std::uint32_t tri_count = 0;

                  const auto count_metrics = []<typename T>(const std::vector<T>& segments) {
                     std::uint32_t count = 0;

                     for (const auto& segment : segments) {
                        if (segment.topology == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST) {
                           count += segment.index_count / 3u;
                        }
                        else {
                           count += segment.index_count + 2u;
                        }
                     }

                     return count;
                  };

                  tri_count += count_metrics(model.opaque_segments);
                  tri_count += count_metrics(model.doublesided_segments);
                  tri_count += count_metrics(model.hardedged_segments);
                  tri_count += count_metrics(model.hardedged_doublesided_segments);

                  ImGui::Text("Triangle Count: %u", tri_count);
                  ImGui::Text("Opaque Segment: %u", model.opaque_segments.size());
                  ImGui::Text("Doublesided Segment: %u",
                              model.doublesided_segments.size());
                  ImGui::Text("Hardedged Segments: %u",
                              model.hardedged_segments.size());
                  ImGui::Text("Hardedged Doublesided Segments: %u",
                              model.hardedged_doublesided_segments.size());

                  ImGui::Text("BBOX Min: %f %f %f", model.bbox.min.x,
                              model.bbox.min.y, model.bbox.min.z);
                  ImGui::Text("BBOX Max: %f %f %f", model.bbox.max.x,
                              model.bbox.max.y, model.bbox.max.z);

                  static float preview_angle = 0.0f;

                  ImGui::SliderAngle("Preview Rotation", &preview_angle);

                  ImGui::Separator();

                  const ImVec2 preview_size_flt = ImGui::GetContentRegionAvail();

                  const UINT preview_width =
                     static_cast<UINT>(std::max(preview_size_flt.x, 64.0f));
                  const UINT preview_height =
                     static_cast<UINT>(std::max(preview_size_flt.y, 64.0f));

                  if (_debug_model_preview_width != preview_width ||
                      _debug_model_preview_height != preview_height) {
                     Com_ptr<ID3D11Texture2D> depth_stencil_texture;

                     const D3D11_TEXTURE2D_DESC depth_stencil_desc = {
                        .Width = preview_width,
                        .Height = preview_height,
                        .MipLevels = 1,
                        .ArraySize = 1,
                        .Format = DXGI_FORMAT_D16_UNORM,
                        .SampleDesc = {1, 0},
                        .Usage = D3D11_USAGE_DEFAULT,
                        .BindFlags = D3D11_BIND_DEPTH_STENCIL,
                     };

                     if (FAILED(_device->CreateTexture2D(&depth_stencil_desc, nullptr,
                                                         depth_stencil_texture.clear_and_assign()))) {
                        log_and_terminate(
                           "Failed to create debug model draw resource.");
                     }

                     if (FAILED(_device->CreateDepthStencilView(
                            depth_stencil_texture.get(), nullptr,
                            _debug_model_preview_dsv.clear_and_assign()))) {
                        log_and_terminate(
                           "Failed to create debug model draw resource.");
                     }

                     Com_ptr<ID3D11Texture2D> render_target_texture;

                     const D3D11_TEXTURE2D_DESC render_target_desc = {
                        .Width = preview_width,
                        .Height = preview_height,
                        .MipLevels = 1,
                        .ArraySize = 1,
                        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                        .SampleDesc = {1, 0},
                        .Usage = D3D11_USAGE_DEFAULT,
                        .BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
                     };

                     if (FAILED(_device->CreateTexture2D(&render_target_desc, nullptr,
                                                         render_target_texture.clear_and_assign()))) {
                        log_and_terminate(
                           "Failed to create debug model draw resource.");
                     }

                     if (FAILED(_device->CreateRenderTargetView(
                            render_target_texture.get(), nullptr,
                            _debug_model_preview_rtv.clear_and_assign()))) {
                        log_and_terminate(
                           "Failed to create debug model draw resource.");
                     }

                     if (FAILED(_device->CreateShaderResourceView(
                            render_target_texture.get(), nullptr,
                            _debug_model_preview_srv.clear_and_assign()))) {
                        log_and_terminate(
                           "Failed to create debug model draw resource.");
                     }

                     _debug_model_preview_width = preview_width;
                     _debug_model_preview_height = preview_height;
                  }

                  _debug_model_draw.draw(dc, model, preview_angle,
                                         _index_buffer.get(), _vertex_buffer.get(),
                                         {
                                            .Width = static_cast<float>(preview_width),
                                            .Height = static_cast<float>(preview_height),
                                            .MinDepth = 0.0f,
                                            .MaxDepth = 1.0f,
                                         },
                                         _debug_model_preview_rtv.get(),
                                         _debug_model_preview_dsv.get());

                  ImGui::GetWindowDrawList()->AddImage(
                     reinterpret_cast<ImTextureID>(_debug_model_preview_srv.get()),
                     ImGui::GetCursorScreenPos(),
                     {
                        ImGui::GetCursorScreenPos().x + static_cast<float>(preview_width),
                        ImGui::GetCursorScreenPos().y + static_cast<float>(preview_height),
                     });
               }

               ImGui::EndChild();

               ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Game Models")) {
               static std::uint32_t selected_game_model_hash = 0;
               std::uint32_t selected_game_model_index = UINT32_MAX;

               if (ImGui::BeginChild("##list",
                                     {ImGui::GetContentRegionAvail().x * 0.4f, 0.0f},
                                     ImGuiChildFlags_ResizeX |
                                        ImGuiChildFlags_FrameStyle)) {
                  for (const auto& [game_model_name_hash, game_model_index] :
                       _game_models_index) {
                     const bool selected =
                        selected_game_model_hash == game_model_name_hash;

                     const char* name = _name_table.lookup(game_model_name_hash);

                     if (ImGui::Selectable(name, selected)) {
                        selected_game_model_hash = game_model_name_hash;
                     }

                     ImGui::SetItemTooltip(name);

                     if (selected) selected_game_model_index = game_model_index;
                  }
               }

               ImGui::EndChild();

               ImGui::SameLine();

               if (ImGui::BeginChild("##model") &&
                   selected_game_model_index < _game_models.size()) {
                  const Game_model& game_model =
                     _game_models[selected_game_model_index];

                  const auto model_name = [&](std::size_t index) {
                     for (const auto& [model_name_hash, model_index] : _models_index) {
                        if (model_index == index)
                           return _name_table.lookup(model_name_hash);
                     }

                     return "<unknown>";
                  };

                  ImGui::Text("LOD0: %s", model_name(game_model.lod0_index));
                  ImGui::Text("LOD1: %s", model_name(game_model.lod1_index));
                  ImGui::Text("LOD2: %s", model_name(game_model.lod2_index));
                  ImGui::Text("LOWD: %s", model_name(game_model.lowd_index));
               }

               ImGui::EndChild();

               ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Entity Classes")) {
               static std::uint32_t selected_entity_class_hash = 0;
               std::uint32_t selected_entity_class_index = UINT32_MAX;

               if (ImGui::BeginChild("##list",
                                     {ImGui::GetContentRegionAvail().x * 0.4f, 0.0f},
                                     ImGuiChildFlags_ResizeX |
                                        ImGuiChildFlags_FrameStyle)) {
                  for (const auto& [entity_class_name_hash, entity_class_index] :
                       _entity_classes_index) {
                     const bool selected =
                        selected_entity_class_hash == entity_class_name_hash;

                     const char* name = _name_table.lookup(entity_class_name_hash);

                     if (ImGui::Selectable(name, selected)) {
                        selected_entity_class_hash = entity_class_name_hash;
                     }

                     ImGui::SetItemTooltip(name);

                     if (selected)
                        selected_entity_class_index = entity_class_index;
                  }
               }

               ImGui::EndChild();

               ImGui::SameLine();

               if (ImGui::BeginChild("##model") &&
                   selected_entity_class_index < _entity_classes.size()) {
                  const Entity_class& entity_class =
                     _entity_classes[selected_entity_class_index];

                  const auto game_model_name = [&](std::size_t index) {
                     for (const auto& [game_model_name_hash, game_model_index] :
                          _models_index) {
                        if (game_model_index == index)
                           return _name_table.lookup(game_model_name_hash);
                     }

                     return "<unknown>";
                  };

                  ImGui::Text("GeometryName: %s",
                              game_model_name(entity_class.game_model_index));
               }

               ImGui::EndChild();

               ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Object Instances")) {
               static std::uint32_t selected_object_instance_index = UINT32_MAX;

               if (ImGui::BeginChild("##list",
                                     {ImGui::GetContentRegionAvail().x * 0.4f, 0.0f},
                                     ImGuiChildFlags_ResizeX |
                                        ImGuiChildFlags_FrameStyle)) {
                  for (std::uint32_t i = 0; i < _object_instances.size(); ++i) {
                     ImGui::PushID(i);

                     const Object_instance& object_instance = _object_instances[i];

                     const bool selected = selected_object_instance_index == i;

                     const char* name = _name_table.lookup(object_instance.name_hash);
                     const char* layer_name =
                        _name_table.lookup(object_instance.layer_name_hash);

                     const ImVec2 cursor_positon = ImGui::GetCursorPos();

                     if (ImGui::Selectable("##select", selected)) {
                        selected_object_instance_index = i;
                     }

                     ImGui::SetItemTooltip("%s:%s", name, layer_name);

                     ImGui::SetCursorPos(cursor_positon);

                     ImGui::Text("%s:%s", name, layer_name);

                     ImGui::PopID();
                  }
               }

               ImGui::EndChild();

               ImGui::SameLine();

               if (ImGui::BeginChild("##instance") &&
                   selected_object_instance_index < _object_instances.size()) {
                  const Object_instance& object_instance =
                     _object_instances[selected_object_instance_index];

                  const auto game_model_name = [&](std::size_t index) {
                     for (const auto& [game_model_name_hash, game_model_index] :
                          _game_models_index) {
                        if (game_model_index == index)
                           return _name_table.lookup(game_model_name_hash);
                     }

                     return "<unknown>";
                  };

                  ImGui::Text("GeometryName: %s",
                              game_model_name(object_instance.game_model_index));

                  ImGui::Text("Rotation X: %f %f %f",
                              object_instance.rotation[0].x,
                              object_instance.rotation[0].y,
                              object_instance.rotation[0].z);

                  ImGui::Text("Rotation Y: %f %f %f",
                              object_instance.rotation[1].x,
                              object_instance.rotation[1].y,
                              object_instance.rotation[1].z);

                  ImGui::Text("Rotation Z: %f %f %f",
                              object_instance.rotation[2].x,
                              object_instance.rotation[2].y,
                              object_instance.rotation[2].z);

                  ImGui::Text("Position: %f %f %f", object_instance.positionWS.x,
                              object_instance.positionWS.y,
                              object_instance.positionWS.z);

                  ImGui::Text("From Child lvl: %i",
                              int{object_instance.from_child_lvl});
               }

               ImGui::EndChild();

               ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("World Preview")) {
               static float camera_yaw = 0.0f;
               static float camera_pitch = 0.0f;
               static glm::vec3 camera_positionWS = {};

               ImGui::PushItemWidth(
                  (ImGui::CalcItemWidth() - ImGui::GetStyle().ItemSpacing.x) * 0.5f);

               ImGui::SliderAngle("##preview_yaw", &camera_yaw, -360.0f, 360.0f,
                                  "Yaw: %.0f deg");

               ImGui::SameLine();

               ImGui::SliderAngle("##preview_pitch", &camera_pitch, -360.0f,
                                  360.0f, "Pitch: %.0f deg");

               ImGui::PopItemWidth();

               ImGui::DragFloat3("Camera Position", &camera_positionWS.x);

               ImGui::Separator();

               const ImVec2 preview_size_flt = ImGui::GetContentRegionAvail();

               const UINT preview_width =
                  static_cast<UINT>(std::max(preview_size_flt.x, 64.0f));
               const UINT preview_height =
                  static_cast<UINT>(std::max(preview_size_flt.y, 64.0f));

               if (_debug_world_preview_width != preview_width ||
                   _debug_world_preview_height != preview_height) {
                  Com_ptr<ID3D11Texture2D> depth_stencil_texture;

                  const D3D11_TEXTURE2D_DESC depth_stencil_desc = {
                     .Width = preview_width,
                     .Height = preview_height,
                     .MipLevels = 1,
                     .ArraySize = 1,
                     .Format = DXGI_FORMAT_D32_FLOAT,
                     .SampleDesc = {1, 0},
                     .Usage = D3D11_USAGE_DEFAULT,
                     .BindFlags = D3D11_BIND_DEPTH_STENCIL,
                  };

                  if (FAILED(_device->CreateTexture2D(&depth_stencil_desc, nullptr,
                                                      depth_stencil_texture.clear_and_assign()))) {
                     log_and_terminate(
                        "Failed to create debug world draw resource.");
                  }

                  if (FAILED(_device->CreateDepthStencilView(
                         depth_stencil_texture.get(), nullptr,
                         _debug_world_preview_dsv.clear_and_assign()))) {
                     log_and_terminate(
                        "Failed to create debug world draw resource.");
                  }

                  Com_ptr<ID3D11Texture2D> render_target_texture;

                  const D3D11_TEXTURE2D_DESC render_target_desc = {
                     .Width = preview_width,
                     .Height = preview_height,
                     .MipLevels = 1,
                     .ArraySize = 1,
                     .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                     .SampleDesc = {1, 0},
                     .Usage = D3D11_USAGE_DEFAULT,
                     .BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
                  };

                  if (FAILED(_device->CreateTexture2D(&render_target_desc, nullptr,
                                                      render_target_texture.clear_and_assign()))) {
                     log_and_terminate(
                        "Failed to create debug world draw resource.");
                  }

                  if (FAILED(_device->CreateRenderTargetView(
                         render_target_texture.get(), nullptr,
                         _debug_world_preview_rtv.clear_and_assign()))) {
                     log_and_terminate(
                        "Failed to create debug world draw resource.");
                  }

                  if (FAILED(_device->CreateShaderResourceView(
                         render_target_texture.get(), nullptr,
                         _debug_world_preview_srv.clear_and_assign()))) {
                     log_and_terminate(
                        "Failed to create debug world draw resource.");
                  }

                  const D3D11_TEXTURE2D_DESC pick_render_target_desc = {
                     .Width = preview_width,
                     .Height = preview_height,
                     .MipLevels = 1,
                     .ArraySize = 1,
                     .Format = DXGI_FORMAT_R32_UINT,
                     .SampleDesc = {1, 0},
                     .Usage = D3D11_USAGE_DEFAULT,
                     .BindFlags = D3D11_BIND_RENDER_TARGET,
                  };

                  if (FAILED(_device->CreateTexture2D(&pick_render_target_desc, nullptr,
                                                      _debug_world_preview_pick
                                                         .clear_and_assign()))) {
                     log_and_terminate(
                        "Failed to create debug world draw resource.");
                  }

                  if (FAILED(_device->CreateRenderTargetView(
                         _debug_world_preview_pick.get(), nullptr,
                         _debug_world_preview_pick_rtv.clear_and_assign()))) {
                     log_and_terminate(
                        "Failed to create debug world draw resource.");
                  }

                  const D3D11_TEXTURE1D_DESC readback_desc{
                     .Width = 1,
                     .MipLevels = 1,
                     .ArraySize = 1,
                     .Format = DXGI_FORMAT_R32_UINT,
                     .Usage = D3D11_USAGE_STAGING,
                     .CPUAccessFlags = D3D11_CPU_ACCESS_READ,
                  };

                  for (Com_ptr<ID3D11Texture1D>& buffer :
                       _debug_world_preview_pick_readback) {
                     if (FAILED(_device->CreateTexture1D(&readback_desc, nullptr,
                                                         buffer.clear_and_assign()))) {
                        log_and_terminate(
                           "Failed to create debug world draw resource.");
                     }
                  }

                  _debug_world_preview_width = preview_width;
                  _debug_world_preview_height = preview_height;
               }

               _debug_world_draw
                  .draw(dc,
                        {
                           .models = _models,
                           .game_models = _game_models,
                           .object_instances = _object_instances,

                        },
                        camera_yaw, camera_pitch, camera_positionWS,
                        _index_buffer.get(), _vertex_buffer.get(),
                        {
                           .viewport =
                              {
                                 .Width = static_cast<float>(preview_width),
                                 .Height = static_cast<float>(preview_height),
                                 .MinDepth = 0.0f,
                                 .MaxDepth = 1.0f,
                              },
                           .rtv = _debug_world_preview_rtv.get(),
                           .picking_rtv = _debug_world_preview_pick_rtv.get(),
                           .dsv = _debug_world_preview_dsv.get(),
                        });

               const ImVec2 image_bottom_right = {
                  ImGui::GetCursorScreenPos().x + static_cast<float>(preview_width),
                  ImGui::GetCursorScreenPos().y + static_cast<float>(preview_height),
               };

               ImGui::GetWindowDrawList()->AddImage(reinterpret_cast<ImTextureID>(
                                                       _debug_world_preview_srv.get()),
                                                    ImGui::GetCursorScreenPos(),
                                                    image_bottom_right);

               const ImVec2 mouse_position = ImGui::GetMousePos();

               if (mouse_position.x >= ImGui::GetCursorScreenPos().x &&
                   mouse_position.x < image_bottom_right.x &&
                   mouse_position.y >= ImGui::GetCursorScreenPos().y &&
                   mouse_position.y < image_bottom_right.x) {

                  const UINT pick_position_x =
                     std::clamp(static_cast<UINT>(mouse_position.x -
                                                  ImGui::GetCursorScreenPos().x),
                                0u, _debug_world_preview_width - 1u);
                  const UINT pick_position_y =
                     std::clamp(static_cast<UINT>(mouse_position.y -
                                                  ImGui::GetCursorScreenPos().y),
                                0u, _debug_world_preview_height - 1u);

                  ID3D11Texture1D* readback_resource =
                     _debug_world_preview_pick_readback[_debug_world_preview_pick_readback_index %
                                                        _debug_world_preview_pick_readback
                                                           .size()]
                        .get();

                  D3D11_MAPPED_SUBRESOURCE mapped{};

                  if (FAILED(dc.Map(readback_resource, 0, D3D11_MAP_READ, 0, &mapped))) {
                     log_and_terminate(
                        "Failed to readback debug world draw resource");
                  }

                  std::uint32_t index = UINT32_MAX;

                  std::memcpy(&index, mapped.pData, sizeof(index));

                  dc.Unmap(readback_resource, 0);

                  const D3D11_BOX box{
                     .left = pick_position_x,
                     .top = pick_position_y,
                     .front = 0,
                     .right = pick_position_x + 1,
                     .bottom = pick_position_y + 1,
                     .back = 1,
                  };

                  dc.CopySubresourceRegion(readback_resource, 0, 0, 0, 0,
                                           _debug_world_preview_pick.get(), 0, &box);

                  _debug_world_preview_pick_readback_index += 1;

                  if (index < _object_instances.size()) {
                     ImGui::SetTooltip(
                        _name_table.lookup(_object_instances[index].name_hash));
                  }
               }

               ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Textures")) {
               _texture_table.show_imgui_page(_name_table);

               ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Name Table")) {
               ImGui::PushItemWidth(ImGui::CalcTextSize("0x00000000").x * 2.0f);

               ImGui::LabelText("Hash", "Name");

               ImGui::Separator();

               for (const auto& [id, name] : _name_table) {
                  ImGui::LabelText(name, "0x%.8x", id);
               }

               ImGui::PopItemWidth();

               ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
         }
      }

      ImGui::End();
   }

private:
   std::shared_mutex _mutex;

   bool _active_rebuild_needed = false;

   Com_ptr<ID3D11Device2> _device;

   Mesh_buffer _index_buffer{*_device, D3D11_BIND_INDEX_BUFFER, MESH_BUFFER_SIZE};
   Mesh_buffer _vertex_buffer{*_device, D3D11_BIND_VERTEX_BUFFER, MESH_BUFFER_SIZE};

   Mesh_copy_queue _mesh_copy_queue{*_device};

   std::vector<Model> _models;
   absl::flat_hash_map<std::uint32_t, std::size_t> _models_index;

   std::vector<Game_model> _game_models;
   absl::flat_hash_map<std::uint32_t, std::size_t> _game_models_index;

   std::vector<Entity_class> _entity_classes;
   absl::flat_hash_map<std::uint32_t, std::size_t> _entity_classes_index;

   std::vector<Object_instance> _object_instances;

   Active_world _active_world;

   Draw_resources _draw_resources;

   Texture_table _texture_table;

   Name_table _name_table;

   std::uint32_t _counter_total_draws = 0;
   std::uint32_t _counter_total_instances = 0;

   Debug_model_draw _debug_model_draw{*_device};
   Debug_world_draw _debug_world_draw{*_device};
   Debug_world_textured_draw _debug_world_textured_draw{*_device};
   Debug_world_aabb_draw _debug_world_aabb_draw{*_device};

   UINT _debug_model_preview_width = 0;
   UINT _debug_model_preview_height = 0;

   Com_ptr<ID3D11DepthStencilView> _debug_model_preview_dsv;
   Com_ptr<ID3D11RenderTargetView> _debug_model_preview_rtv;
   Com_ptr<ID3D11ShaderResourceView> _debug_model_preview_srv;

   UINT _debug_world_preview_width = 0;
   UINT _debug_world_preview_height = 0;

   Com_ptr<ID3D11DepthStencilView> _debug_world_preview_dsv;
   Com_ptr<ID3D11RenderTargetView> _debug_world_preview_rtv;
   Com_ptr<ID3D11ShaderResourceView> _debug_world_preview_srv;
   Com_ptr<ID3D11Texture2D> _debug_world_preview_pick;
   Com_ptr<ID3D11RenderTargetView> _debug_world_preview_pick_rtv;

   std::size_t _debug_world_preview_pick_readback_index = 0;
   std::array<Com_ptr<ID3D11Texture1D>, 2> _debug_world_preview_pick_readback;

   void add_model_segment(const std::string_view model_debug_name,
                          const Model_merge_segment& merge_segment,
                          std::vector<Model_segment>& model_output) noexcept
   {
      const UINT vertex_buffer_element_size =
         sizeof(decltype(merge_segment.vertices)::value_type);
      UINT vertex_buffer_byte_offset = 0;

      if (FAILED(_vertex_buffer.allocate(merge_segment.vertices.size() * vertex_buffer_element_size,
                                         vertex_buffer_element_size,
                                         vertex_buffer_byte_offset))) {
         log(Log_level::error, "Ran out of Shadow World vertex buffer space! (model: {})",
             model_debug_name);

         return;
      }

      UINT index_buffer_byte_offset = 0;

      if (FAILED(_index_buffer.allocate(merge_segment.indices.size() * sizeof(std::uint16_t),
                                        sizeof(std::uint16_t),
                                        index_buffer_byte_offset))) {
         log(Log_level::error, "Ran out of Shadow World index buffer space! (model: {})",
             model_debug_name);

         return;
      }

      _mesh_copy_queue.queue(std::as_bytes(std::span{merge_segment.vertices}),
                             *_vertex_buffer.get(), vertex_buffer_byte_offset);

      _mesh_copy_queue.queue(std::as_bytes(std::span{merge_segment.indices}),
                             *_index_buffer.get(), index_buffer_byte_offset);

      model_output.push_back({
         .topology = merge_segment.topology,
         .index_count = merge_segment.indices.size(),
         .start_index = index_buffer_byte_offset / sizeof(std::uint16_t),
         .base_vertex = static_cast<INT>(vertex_buffer_byte_offset / vertex_buffer_element_size),
      });
   }

   void add_model_segment(const std::string_view model_debug_name,
                          const Model_merge_segment_hardedged& merge_segment,
                          std::vector<Model_segment_hardedged>& model_output) noexcept
   {
      const UINT vertex_buffer_element_size =
         sizeof(decltype(merge_segment.vertices)::value_type);
      UINT vertex_buffer_byte_offset = 0;

      if (FAILED(_vertex_buffer.allocate(merge_segment.vertices.size() * vertex_buffer_element_size,
                                         vertex_buffer_element_size,
                                         vertex_buffer_byte_offset))) {
         log(Log_level::error, "Ran out of Shadow World vertex buffer space! (model: {})",
             model_debug_name);

         return;
      }

      UINT index_buffer_byte_offset = 0;

      if (FAILED(_index_buffer.allocate(merge_segment.indices.size() * sizeof(std::uint16_t),
                                        sizeof(std::uint16_t),
                                        index_buffer_byte_offset))) {
         log(Log_level::error, "Ran out of Shadow World index buffer space! (model: {})",
             model_debug_name);

         return;
      }

      _mesh_copy_queue.queue(std::as_bytes(std::span{merge_segment.vertices}),
                             *_vertex_buffer.get(), vertex_buffer_byte_offset);

      _mesh_copy_queue.queue(std::as_bytes(std::span{merge_segment.indices}),
                             *_index_buffer.get(), index_buffer_byte_offset);

      model_output.push_back({
         .topology = merge_segment.topology,
         .index_count = merge_segment.indices.size(),
         .start_index = index_buffer_byte_offset / sizeof(std::uint16_t),
         .base_vertex = static_cast<INT>(vertex_buffer_byte_offset / vertex_buffer_element_size),

         .texture_index = _texture_table.acquire(merge_segment.texture_name_hash),
      });
   }

   void build_active_world() noexcept
   {
      _active_world.build(*_device, _models, _game_models, _object_instances);
      _active_rebuild_needed = false;
   }
};

std::atomic<Shadow_world*> shadow_world_ptr = nullptr;

}

Shadow_world_interface shadow_world;

void Shadow_world_interface::initialize(ID3D11Device2& device, shader::Database& shaders)
{
   if (shadow_world_ptr.load() != nullptr) {
      log(Log_level::warning, "Attempt to initialize Shadow World twice.");

      return;
   }

   static Shadow_world shadow_world_impl{device, shaders};

   shadow_world_ptr.store(&shadow_world_impl);
}

void Shadow_world_interface::process_mesh_copy_queue(ID3D11DeviceContext2& dc) noexcept
{
   Shadow_world* self = shadow_world_ptr.load(std::memory_order_relaxed);

   if (!self) return;

   self->process_mesh_copy_queue(dc);
}

void Shadow_world_interface::draw_shadow_views(ID3D11DeviceContext2& dc,
                                               const glm::mat4& view_proj_matrix,
                                               std::span<const Shadow_draw_view> views) noexcept
{
   Shadow_world* self = shadow_world_ptr.load(std::memory_order_relaxed);

   if (!self) return;

   self->draw_shadow_views(dc, view_proj_matrix, views);
}

void Shadow_world_interface::draw_shadow_world_preview(
   ID3D11DeviceContext2& dc, const glm::mat4& projection_matrix,
   const D3D11_VIEWPORT& viewport, ID3D11RenderTargetView* rtv,
   ID3D11DepthStencilView* dsv) noexcept
{
   Shadow_world* self = shadow_world_ptr.load(std::memory_order_relaxed);

   if (!self) return;

   self->draw_shadow_world_preview(dc, projection_matrix, viewport, rtv, dsv);
}

void Shadow_world_interface::draw_shadow_world_textured_preview(
   ID3D11DeviceContext2& dc, const glm::mat4& projection_matrix,
   const D3D11_VIEWPORT& viewport, ID3D11RenderTargetView* rtv,
   ID3D11DepthStencilView* dsv) noexcept
{
   Shadow_world* self = shadow_world_ptr.load(std::memory_order_relaxed);

   if (!self) return;

   self->draw_shadow_world_textured_preview(dc, projection_matrix, viewport, rtv, dsv);
}

void Shadow_world_interface::draw_shadow_world_aabb_overlay(
   ID3D11DeviceContext2& dc, const glm::mat4& projection_matrix,
   const D3D11_VIEWPORT& viewport, ID3D11RenderTargetView* rtv,
   ID3D11DepthStencilView* dsv) noexcept
{
   Shadow_world* self = shadow_world_ptr.load(std::memory_order_relaxed);

   if (!self) return;

   self->draw_shadow_world_aabb_overlay(dc, projection_matrix, viewport, rtv, dsv);
}

void Shadow_world_interface::clear() noexcept
{
   Shadow_world* self = shadow_world_ptr.load(std::memory_order_relaxed);

   if (!self) return;

   self->clear();
}

void Shadow_world_interface::add_texture(const Input_texture& texture) noexcept
{
   Shadow_world* self = shadow_world_ptr.load(std::memory_order_relaxed);

   if (!self) return;

   self->add_texture(texture);
}

void Shadow_world_interface::add_model(const Input_model& model) noexcept
{
   Shadow_world* self = shadow_world_ptr.load(std::memory_order_relaxed);

   if (!self) return;

   self->add_model(model);
}

void Shadow_world_interface::add_game_model(const Input_game_model& game_model) noexcept
{
   Shadow_world* self = shadow_world_ptr.load(std::memory_order_relaxed);

   if (!self) return;

   self->add_game_model(game_model);
}

void Shadow_world_interface::add_entity_class(const Input_entity_class& entity_class) noexcept
{
   Shadow_world* self = shadow_world_ptr.load(std::memory_order_relaxed);

   if (!self) return;

   self->add_entity_class(entity_class);
}

void Shadow_world_interface::add_object_instance(const Input_object_instance& instance) noexcept
{
   Shadow_world* self = shadow_world_ptr.load(std::memory_order_relaxed);

   if (!self) return;

   self->add_object_instance(instance);
}

void Shadow_world_interface::register_texture(ID3D11ShaderResourceView& srv,
                                              const Texture_hash& data_hash) noexcept
{
   Shadow_world* self = shadow_world_ptr.load(std::memory_order_relaxed);

   if (!self) return;

   self->register_texture(srv, data_hash);
}

void Shadow_world_interface::unregister_texture(ID3D11ShaderResourceView& srv) noexcept
{
   Shadow_world* self = shadow_world_ptr.load(std::memory_order_relaxed);

   if (!self) return;

   self->unregister_texture(srv);
}

void Shadow_world_interface::show_imgui(ID3D11DeviceContext2& dc) noexcept
{
   Shadow_world* self = shadow_world_ptr.load(std::memory_order_relaxed);

   if (!self) return;

   self->show_imgui(dc);
}

}