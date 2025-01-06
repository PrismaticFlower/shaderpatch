#include "shadow_world.hpp"

#include "internal/mesh_buffer.hpp"
#include "internal/mesh_copy_queue.hpp"
#include "internal/model.hpp"
#include "internal/name_table.hpp"

#include "../imgui/imgui.h"
#include "../logger.hpp"

#include "com_ptr.hpp"
#include "swbf_fnv_1a.hpp"

#include <d3d11_2.h>

#include <shared_mutex>

namespace sp::shadows {

namespace {

const UINT MESH_BUFFER_SIZE = 0x4000000;

// One less than UINT16_MAX to avoid the trianlge strip cut value.
const UINT MAX_SEGMENT_VERTICES = UINT16_MAX - 1;

struct Shadow_world {
   Shadow_world(ID3D11Device2& device) : _device{copy_raw_com_ptr(device)} {}

   void process_mesh_copy_queue(ID3D11DeviceContext2& dc) noexcept
   {
      _mesh_copy_queue.process(dc);
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
   }

   void add_model(const Input_model& input_model) noexcept
   {
      std::scoped_lock lock{_mutex};

      if (input_model.segments.empty()) {
         log_fmt(Log_level::info, "Discarding model '{}' with no usable segments for shadows. (Is either skinned or fully transparent).",
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
         log_debug("Skipping adding model '{}' as a model with the name hash "
                   "already exists.",
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
               const UINT vertex_buffer_element_size =
                  sizeof(decltype(merge_segment.vertices)::value_type);
               UINT vertex_buffer_byte_offset = 0;

               if (FAILED(_vertex_buffer.allocate(merge_segment.vertices.size() *
                                                     vertex_buffer_element_size,
                                                  vertex_buffer_element_size,
                                                  vertex_buffer_byte_offset))) {
                  log(Log_level::error, "Ran out of Shadow World vertex buffer space! (model: {})",
                      input_model.name);

                  return;
               }

               UINT index_buffer_byte_offset = 0;

               if (FAILED(_index_buffer.allocate(merge_segment.indices.size() *
                                                    sizeof(std::uint16_t),
                                                 sizeof(std::uint16_t),
                                                 index_buffer_byte_offset))) {
                  log(Log_level::error,
                      "Ran out of Shadow World index buffer space! (model: {})",
                      input_model.name);

                  return;
               }

               _mesh_copy_queue.queue(std::as_bytes(std::span{merge_segment.vertices}),
                                      *_vertex_buffer.get(),
                                      vertex_buffer_byte_offset);

               _mesh_copy_queue.queue(std::as_bytes(std::span{merge_segment.indices}),
                                      *_index_buffer.get(), index_buffer_byte_offset);

               std::vector<Model_segment_hardedged>& model_output =
                  segment.doublesided ? model.hardedged_doublesided_segments
                                      : model.hardedged_segments;

               model_output.push_back({
                  .topology = topology,
                  .index_count = merge_segment.indices.size(),
                  .start_index = index_buffer_byte_offset / sizeof(std::uint16_t),
                  .base_vertex = static_cast<INT>(vertex_buffer_byte_offset /
                                                  vertex_buffer_element_size),

                  .texture = nullptr,
               });

               merge_segment.indices.clear();
               merge_segment.vertices.clear();
               merge_segment.topology = topology;
               merge_segment.texture_name_hash = 0;
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

            merge_segment.indices.reserve(merge_segment.indices.size() +
                                          segment.indices.size());

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
               const UINT vertex_buffer_element_size =
                  sizeof(decltype(merge_segment.vertices)::value_type);
               UINT vertex_buffer_byte_offset = 0;

               if (FAILED(_vertex_buffer.allocate(merge_segment.vertices.size() *
                                                     vertex_buffer_element_size,
                                                  vertex_buffer_element_size,
                                                  vertex_buffer_byte_offset))) {
                  log(Log_level::error, "Ran out of Shadow World vertex buffer space! (model: {})",
                      input_model.name);

                  return;
               }

               UINT index_buffer_byte_offset = 0;

               if (FAILED(_index_buffer.allocate(merge_segment.indices.size() *
                                                    sizeof(std::uint16_t),
                                                 sizeof(std::uint16_t),
                                                 index_buffer_byte_offset))) {
                  log(Log_level::error,
                      "Ran out of Shadow World index buffer space! (model: {})",
                      input_model.name);

                  return;
               }

               _mesh_copy_queue.queue(std::as_bytes(std::span{merge_segment.vertices}),
                                      *_vertex_buffer.get(),
                                      vertex_buffer_byte_offset);

               _mesh_copy_queue.queue(std::as_bytes(std::span{merge_segment.indices}),
                                      *_index_buffer.get(), index_buffer_byte_offset);

               std::vector<Model_segment>& model_output =
                  segment.doublesided ? model.doublesided_segments
                                      : model.opaque_segments;

               model_output.push_back({
                  .topology = topology,
                  .index_count = merge_segment.indices.size(),
                  .start_index = index_buffer_byte_offset / sizeof(std::uint16_t),
                  .base_vertex = static_cast<INT>(vertex_buffer_byte_offset /
                                                  vertex_buffer_element_size),
               });

               merge_segment.indices.clear();
               merge_segment.vertices.clear();
               merge_segment.topology = topology;
            }

            const std::size_t index_offset = merge_segment.vertices.size();

            merge_segment.vertices.insert(merge_segment.vertices.end(),
                                          segment.vertices.begin(),
                                          segment.vertices.end());

            merge_segment.indices.reserve(merge_segment.indices.size() +
                                          segment.indices.size());

            for (const std::uint16_t index : segment.indices) {
               merge_segment.indices.push_back(
                  static_cast<std::uint16_t>(index + index_offset));
            }
         }
      }

      const std::size_t model_index = _models.size();

      _models_index.emplace(name_hash, model_index);
      _models.push_back(std::move(model));
   }

   void add_game_model(const Input_game_model& game_model) noexcept
   {
      std::scoped_lock lock{_mutex};

      (void)game_model;
   }

   void add_object(const Input_object_class& object_class) noexcept
   {
      std::scoped_lock lock{_mutex};

      (void)object_class;
   }

   void add_object_instance(const Input_instance& instance) noexcept
   {
      std::scoped_lock lock{_mutex};

      (void)instance;
   }

   void show_imgui(ID3D11DeviceContext2& dc) noexcept
   {
      (void)dc;

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

   Com_ptr<ID3D11Device2> _device;

   Mesh_buffer _index_buffer{*_device, D3D11_BIND_INDEX_BUFFER, MESH_BUFFER_SIZE};
   Mesh_buffer _vertex_buffer{*_device, D3D11_BIND_VERTEX_BUFFER, MESH_BUFFER_SIZE};

   Mesh_copy_queue _mesh_copy_queue{*_device};

   std::vector<Model> _models;
   absl::flat_hash_map<std::uint32_t, std::size_t> _models_index;

   Name_table _name_table;
};

std::atomic<Shadow_world*> shadow_world_ptr = nullptr;

}

Shadow_world_interface shadow_world;

void Shadow_world_interface::initialize(ID3D11Device2& device)
{
   if (shadow_world_ptr.load() != nullptr) {
      log(Log_level::warning, "Attempt to initialize Shadow World twice.");

      return;
   }

   static Shadow_world shadow_world_impl{device};

   shadow_world_ptr.store(&shadow_world_impl);
}

void Shadow_world_interface::process_mesh_copy_queue(ID3D11DeviceContext2& dc) noexcept
{
   Shadow_world* self = shadow_world_ptr.load(std::memory_order_relaxed);

   if (!self) return;

   self->process_mesh_copy_queue(dc);
}

void Shadow_world_interface::clear() noexcept
{
   Shadow_world* self = shadow_world_ptr.load(std::memory_order_relaxed);

   if (!self) return;

   self->clear();
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

void Shadow_world_interface::add_object(const Input_object_class& object_class) noexcept
{
   Shadow_world* self = shadow_world_ptr.load(std::memory_order_relaxed);

   if (!self) return;

   self->add_object(object_class);
}

void Shadow_world_interface::add_object_instance(const Input_instance& instance) noexcept
{
   Shadow_world* self = shadow_world_ptr.load(std::memory_order_relaxed);

   if (!self) return;

   self->add_object_instance(instance);
}

void Shadow_world_interface::show_imgui(ID3D11DeviceContext2& dc) noexcept
{
   Shadow_world* self = shadow_world_ptr.load(std::memory_order_relaxed);

   if (!self) return;

   self->show_imgui(dc);
}

}