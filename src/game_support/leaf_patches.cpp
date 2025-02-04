#include "leaf_patches.hpp"
#include "game_memory.hpp"
#include "leaf_patch_structures.hpp"

#include "../imgui/imgui.h"

#include <absl/container/flat_hash_map.h>

namespace sp::game_support {

using namespace structures;

namespace {

auto get_leaf_patch_class_data(LeafPatch* object, bool is_debug_executable) noexcept
   -> LeafPatchClass_data&
{
   if (!is_debug_executable) return object->leafPatchClass->LeafPatchClass_data;

   return reinterpret_cast<LeafPatchClass_DebugBuild*>(object->leafPatchClass)->LeafPatchClass_data;
}

}

void acquire_leaf_patches(std::vector<Leaf_patch>& leaf_patches,
                          std::vector<LeafPatchClass_data*>& leaf_patch_classes) noexcept
{
   leaf_patches.clear();
   leaf_patch_classes.clear();

   const Game_memory& game_memory = get_game_memory();

   LeafPatchListNode* leaf_patch_list =
      static_cast<LeafPatchListNode*>(game_memory.leaf_patch_list);

   if (!leaf_patch_list) return;

   LeafPatchListNode* start = leaf_patch_list;
   LeafPatchListNode* node = leaf_patch_list->next;

   static absl::flat_hash_map<LeafPatchClass_data*, std::size_t> class_lookup;

   class_lookup.clear();

   while (node != start) {
      LeafPatch* object = node->object;
      std::size_t class_index = 0;

      LeafPatchClass_data& classData =
         get_leaf_patch_class_data(object, game_memory.is_debug_executable);

      if (auto it = class_lookup.find(&classData); it != class_lookup.end()) {
         class_index = it->second;
      }
      else {
         class_index = leaf_patch_classes.size();

         class_lookup.emplace(&classData, class_index);
         leaf_patch_classes.emplace_back(&classData);
      }

      PblMatrix& transform = object->transform;

      leaf_patches.push_back({
         .class_index = class_index,

         .bounds_radius = classData.boundsRadius,

         .transform = {{
            {transform.rows[0].x, transform.rows[1].x, transform.rows[2].x,
             transform.rows[3].x},
            {transform.rows[0].y, transform.rows[1].y, transform.rows[2].y,
             transform.rows[3].y},
            {transform.rows[0].z, transform.rows[1].z, transform.rows[2].z,
             transform.rows[3].z},
         }},
      });

      node = node->next;
   }
}

void show_leaf_patches_imgui() noexcept
{
   const Game_memory& game_memory = get_game_memory();

   LeafPatchListNode* leaf_patch_list =
      static_cast<LeafPatchListNode*>(game_memory.leaf_patch_list);

   if (!leaf_patch_list) return;

   ImGui::Begin("Game Leaf Patches");

   LeafPatchListNode* start = leaf_patch_list;
   LeafPatchListNode* node = leaf_patch_list->next;

   while (node != start) {
      LeafPatch* object = node->object;

      if (ImGui::TreeNode(object, "%p", object)) {
         if (ImGui::TreeNode("RedSceneObject")) {
            RedSceneObject_data& redSceneObject =
               object->redSceneObject.RedSceneObject_data;

            ImGui::DragFloat3("Render Position",
                              &redSceneObject.m_renderPosition.x);

            ImGui::DragFloat("Render Radius", &redSceneObject.m_renderRadius);

            ImGui::Checkbox("Activated", &redSceneObject.m_activated);

            ImGui::TreePop();
         }

         if (ImGui::TreeNode("CollisionObject")) {
            CollisionObject& collisionObject = object->collisionObject;

            ImGui::DragFloat3("Position", &collisionObject.position.x);

            ImGui::DragFloat("Radius X", &collisionObject.radiusX);

            ImGui::DragFloat("Radius Y", &collisionObject.radiusY);

            ImGui::TreePop();
         }

         if (ImGui::TreeNode("Transform")) {
            PblMatrix& transform = object->transform;

            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);

            ImGui::DragFloat4("##0", &transform.rows[0].x);
            ImGui::DragFloat4("##1", &transform.rows[1].x);
            ImGui::DragFloat4("##2", &transform.rows[2].x);
            ImGui::DragFloat4("##3", &transform.rows[3].x);

            ImGui::PopItemWidth();

            ImGui::TreePop();
         }

         if (ImGui::TreeNode("LeafPatchClass")) {
            LeafPatchClass_data& leafPatchClass =
               get_leaf_patch_class_data(object, game_memory.is_debug_executable);

            ImGui::DragFloat("Bounds Radius", &leafPatchClass.boundsRadius);

            ImGui::Separator();

            ImGui::DragFloat3("BBOX Size", &leafPatchClass.bboxSize.x);
            ImGui::DragFloat3("BBOX Centre", &leafPatchClass.bboxCentre.x);

            ImGui::Separator();

            ImGui::DragScalar("Max Falling Leaves", ImGuiDataType_U8,
                              &leafPatchClass.m_maxFallingLeaves);
            ImGui::DragScalar("Max Scatter Birds", ImGuiDataType_U8,
                              &leafPatchClass.m_maxScatterBirds);

            ImGui::Separator();

            ImGui::DragFloat("Radius", &leafPatchClass.m_radius);
            ImGui::DragFloat("Height Scale", &leafPatchClass.m_heightScale);
            ImGui::DragFloat("Height", &leafPatchClass.m_height);

            ImGui::Separator();

            ImGui::DragInt("Seed", &leafPatchClass.m_seed);

            switch (leafPatchClass.m_type) {
            case LeafPatchType_Default:
               ImGui::LabelText("Type", "Default");
               break;
            case LeafPatchType_Unknown:
               ImGui::LabelText("Type", "Unknown");
               break;
            case LeafPatchType_Box:
               ImGui::LabelText("Type", "Box");
               break;
            case LeafPatchType_Vine:
               ImGui::LabelText("Type", "Vine");
               break;
            }

            if (ImGui::TreeNode("Particles")) {
               for (int i = 0; i < leafPatchClass.m_numParticles; ++i) {
                  ImGui::PushID(i);

                  LeafPatchParticle& particle = leafPatchClass.m_particles[i];

                  ImGui::DragFloat3("Position", &particle.position.x);
                  ImGui::DragFloat("Wiggle", &particle.wiggle);
                  ImGui::DragFloat("Size", &particle.size);
                  ImGui::DragFloat("Wiggle Accum", &particle.wiggleAccum);
                  ImGui::DragScalar("Variation", ImGuiDataType_U8, &particle.variation);
                  ImGui::ColorEdit4("Color", &particle.color.x);

                  ImGui::PopID();

                  ImGui::Separator();
               }

               ImGui::TreePop();
            }

            ImGui::DragFloat3("Offset", &leafPatchClass.m_offset.x);
            ImGui::DragFloat("Min Size", &leafPatchClass.m_minSize);
            ImGui::DragFloat("Max Size", &leafPatchClass.m_maxSize);
            ImGui::DragFloat("Alpha", &leafPatchClass.m_alpha);
            ImGui::DragFloat("Max Distance", &leafPatchClass.m_maxDistance);
            ImGui::DragFloat("Cone Height", &leafPatchClass.m_coneHeight);
            ImGui::DragFloat3("Box Size", &leafPatchClass.m_boxSize.x);
            ImGui::LabelText("Texture", "0x%.8x", leafPatchClass.m_textureHash);
            ImGui::DragFloat("Darkness Min", &leafPatchClass.m_darknessMin);
            ImGui::DragFloat("Darkness Max", &leafPatchClass.m_darknessMax);
            ImGui::DragFloat("Unknown (field24_0x74)", &leafPatchClass.field24_0x74);
            ImGui::DragInt("Num Parts", &leafPatchClass.m_numParts);

            ImGui::Separator();

            ImGui::DragFloat("Vine X", &leafPatchClass.m_vineX);
            ImGui::DragFloat("Vine Y", &leafPatchClass.m_vineY);
            ImGui::DragInt("Vine Length X", &leafPatchClass.m_vineLengthX);
            ImGui::DragInt("Vine Length Y", &leafPatchClass.m_vineLengthY);
            ImGui::DragFloat("Vine Spread", &leafPatchClass.m_vineSpread);

            ImGui::Separator();

            ImGui::DragFloat("Wiggle Speed", &leafPatchClass.m_wiggleSpeed);
            ImGui::DragFloat("Wiggle Amount", &leafPatchClass.m_wiggleAmount);

            ImGui::Separator();

            ImGui::DragInt("Num Visible", &leafPatchClass.m_numVisible);

            ImGui::DragFloat("Num Visible Frac", &leafPatchClass.m_numVisibleFrac);

            if (ImGui::TreeNode("LeafPatchRenderable")) {
               LeafPatchRenderable_data& leafPatchRenderable =
                  leafPatchClass.m_renderable->LeafPatchRenderable_data;

               ImGui::LabelText("Primitive 1", "%p", leafPatchRenderable.primitive);
               ImGui::LabelText("Shader 1", "%p", leafPatchRenderable.m_shader0);
               ImGui::LabelText("Primitive 2", "%p", leafPatchRenderable.primitive2);
               ImGui::LabelText("Shader 2", "%p", leafPatchRenderable.shader1);
               ImGui::LabelText("Index Buffer 1", "%p",
                                leafPatchRenderable.m_indexBuffer);
               ImGui::LabelText("Index Buffer 2", "%p",
                                leafPatchRenderable.m_indexBuffer2);
               ImGui::LabelText("Leaf Catch Class", "%p",
                                leafPatchRenderable.m_leafPatchClass);

               if (ImGui::TreeNode("Heap")) {
                  PblHeap<int, float>* heap = leafPatchRenderable.m_heap;

                  ImGui::LabelText("Size", "%i", heap->size);
                  ImGui::LabelText("Capacity", "%i", heap->capacity);
                  ImGui::LabelText("Unknown Flag", "%i", (int)heap->unknownFlag);

                  ImGui::Separator();

                  for (int i = 0; i < heap->size; ++i) {
                     ImGui::Text("%i: %f", heap->data[i].second, heap->data[i].first);
                  }

                  ImGui::TreePop();
               }

               ImGui::DragScalar("Index Buffer Offset 2", ImGuiDataType_U32,
                                 &leafPatchRenderable.m_indexBuffer2Offset);
               ImGui::DragScalar("Index Buffer Offset", ImGuiDataType_U32,
                                 &leafPatchRenderable.m_indexBufferOffset);

               ImGui::LabelText("Vertex Buffer", "%p", leafPatchRenderable.vertexUpload);

               ImGui::Separator();

               ImGui::DragFloat3("Camera Y Axis",
                                 &leafPatchRenderable.cameraYAxis.x);

               if (ImGui::BeginListBox("Camera XZ Offsets")) {
                  for (PblVector3& offset : leafPatchRenderable.offsetsXZ) {
                     ImGui::PushID(&offset);

                     ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

                     ImGui::DragFloat3("##edit",
                                       &leafPatchRenderable.cameraZAxis.x);

                     ImGui::PopID();
                  }

                  ImGui::EndListBox();
               }

               ImGui::DragFloat3("Camera Z Axis",
                                 &leafPatchRenderable.cameraZAxis.x);

               ImGui::DragScalar("Num Parts Is 4", ImGuiDataType_U32,
                                 &leafPatchRenderable.m_numPartsIs4);

               ImGui::TreePop();
            }

            ImGui::TreePop();
         }

         ImGui::DragFloat("Max Distance", &object->m_maxDistance);

         ImGui::TreePop();
      }

      node = node->next;
   }

   ImGui::End();
}

}