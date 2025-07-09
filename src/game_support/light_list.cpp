#include "light_list.hpp"
#include "game_memory.hpp"
#include "structures/light_list.hpp"

#include "../imgui/imgui.h"

#include <absl/container/flat_hash_map.h>

namespace sp::game_support {

using namespace structures;

void acquire_global_lights(glm::vec3& global_light1_dir, glm::vec3& global_light2_dir) noexcept
{
   const Game_memory& game_memory = get_game_memory();

   if (!game_memory.global_dir_lights) return;

   if (game_memory.global_dir_lights[0]) {
      global_light1_dir =
         {game_memory.global_dir_lights[0]->RedDirectionalLight_data.direction.x,
          game_memory.global_dir_lights[0]->RedDirectionalLight_data.direction.y,
          game_memory.global_dir_lights[0]->RedDirectionalLight_data.direction.z};
   }

   if (game_memory.global_dir_lights[1]) {
      global_light2_dir =
         {game_memory.global_dir_lights[1]->RedDirectionalLight_data.direction.x,
          game_memory.global_dir_lights[1]->RedDirectionalLight_data.direction.y,
          game_memory.global_dir_lights[1]->RedDirectionalLight_data.direction.z};
   }
}

void acquire_light_list(std::vector<Point_light>& point_lights) noexcept
{
   point_lights.clear();

   const Game_memory& game_memory = get_game_memory();

   RedLightList* light_list = game_memory.light_list;

   if (!light_list) return;

   RedLightListNode* start = &light_list->node;
   RedLightListNode* node = light_list->node.next;

   while (node != start) {
      RedLight* light_base = node->light;
      RedLight_data& base_data = light_base->RedLight_data;

      if (base_data.type == RedLightType::Omni) {
         RedOmniLight_data& omni_data = ((RedOmniLight*)light_base)->RedOmniLight_data;

         point_lights.push_back({
            .positionWS = {omni_data.position.x, omni_data.position.y,
                           omni_data.position.z},
            .inv_range_sq = omni_data.invRangeSq,
            .color = {base_data.color.r, base_data.color.g, base_data.color.b},
         });
      }

      node = node->next;
   }
}

void show_light_list_imgui() noexcept
{
   const Game_memory& game_memory = get_game_memory();

   RedLightList* light_list = game_memory.light_list;

   if (!light_list) return;

   ImGui::Begin("Game Lights");

   if (ImGui::CollapsingHeader("Global Lights")) {
      for (int i = 0; i < 1; ++i) {
         if (!game_memory.global_dir_lights[i]) continue;

         if (ImGui::TreeNode(game_memory.global_dir_lights[i], "Global Light %d", i)) {
            RedDirectionalLight* light = game_memory.global_dir_lights[i];

            {
               RedLight_data& data = light->RedLight_data;

               ImGui::SeparatorText("Flags");

               if (ImGui::Selectable("Is Static",
                                     (data.flags & RedLightFlags::IsStatic) != 0)) {
                  data.flags = (RedLightFlags)(data.flags ^ RedLightFlags::IsStatic);
               }

               if (ImGui::Selectable("Cast Shadow",
                                     (data.flags & RedLightFlags::CastShadow) != 0)) {
                  data.flags = (RedLightFlags)(data.flags ^ RedLightFlags::CastShadow);
               }

               if (ImGui::Selectable("Active",
                                     (data.flags & RedLightFlags::Active) != 0)) {
                  data.flags = (RedLightFlags)(data.flags ^ RedLightFlags::Active);
               }

               if (ImGui::Selectable("Cast Specular",
                                     (data.flags & RedLightFlags::CastSpecular) != 0)) {
                  data.flags =
                     (RedLightFlags)(data.flags ^ RedLightFlags::CastSpecular);
               }

               ImGui::InputScalar("Name Hash", ImGuiDataType_U32, &data.nameHash);

               ImGui::InputScalar("Texture Hash", ImGuiDataType_U32, &data.textureHash);

               if (ImGui::BeginCombo("Texture Addressing",
                                     data.textureAddressing == RedLightTextureAddressing::Wrap
                                        ? "Wrap"
                                        : "Clamp")) {
                  if (ImGui::Selectable("Wrap", data.textureAddressing ==
                                                   RedLightTextureAddressing::Wrap)) {
                     data.textureAddressing = RedLightTextureAddressing::Wrap;
                  }

                  if (ImGui::Selectable("Clamp", data.textureAddressing ==
                                                    RedLightTextureAddressing::Clamp)) {
                     data.textureAddressing = RedLightTextureAddressing::Clamp;
                  }

                  ImGui::EndCombo();
               }

               ImGui::ColorEdit3("Color", &data.color.r,
                                 ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
            }

            ImGui::SeparatorText("Directional Light Properties");

            {
               RedDirectionalLight_data& data = light->RedDirectionalLight_data;

               ImGui::DragFloat3("Direction", &data.direction.x);

               ImGui::InputScalar("Region Name Hash", ImGuiDataType_U32,
                                  &data.regionNameHash);

               ImGui::DragFloat4("Texture Matrix 0", &data.textureMatrix.rows[0].x);
               ImGui::DragFloat4("Texture Matrix 1", &data.textureMatrix.rows[1].x);
               ImGui::DragFloat4("Texture Matrix 2", &data.textureMatrix.rows[2].x);
               ImGui::DragFloat4("Texture Matrix 3", &data.textureMatrix.rows[3].x);
               ImGui::Checkbox("Texture Matrix Dirty", &data.textureMatrixDirty);

               ImGui::DragFloat2("Texture Offset", &data.textureOffset.x);
               ImGui::DragFloat2("Texture Tiling", &data.textureTile.x);
            }

            ImGui::TreePop();
         }
      }
   }

   if (ImGui::CollapsingHeader("Light List")) {
      RedLightListNode* start = &light_list->node;
      RedLightListNode* node = light_list->node.next;

      while (node != start) {
         RedLight* light_base = node->light;

         if (ImGui::TreeNode(light_base, "%p", light_base)) {
            if (ImGui::TreeNode("RedLight")) {
               RedLight_data& data = light_base->RedLight_data;

               ImGui::SeparatorText("Flags");

               if (ImGui::Selectable("Is Static",
                                     (data.flags & RedLightFlags::IsStatic) != 0)) {
                  data.flags = (RedLightFlags)(data.flags ^ RedLightFlags::IsStatic);
               }

               if (ImGui::Selectable("Cast Shadow",
                                     (data.flags & RedLightFlags::CastShadow) != 0)) {
                  data.flags = (RedLightFlags)(data.flags ^ RedLightFlags::CastShadow);
               }

               if (ImGui::Selectable("Active",
                                     (data.flags & RedLightFlags::Active) != 0)) {
                  data.flags = (RedLightFlags)(data.flags ^ RedLightFlags::Active);
               }

               if (ImGui::Selectable("Cast Specular",
                                     (data.flags & RedLightFlags::CastSpecular) != 0)) {
                  data.flags =
                     (RedLightFlags)(data.flags ^ RedLightFlags::CastSpecular);
               }

               ImGui::InputScalar("Name Hash", ImGuiDataType_U32, &data.nameHash);

               switch (data.type) {
               case RedLightType::Ambient:
                  ImGui::LabelText("Type", "Ambient");
                  break;
               case RedLightType::Directional:
                  ImGui::LabelText("Type", "Directional");
                  break;
               case RedLightType::Omni:
                  ImGui::LabelText("Type", "Point");
                  break;
               case RedLightType::Spot:
                  ImGui::LabelText("Type", "Spot");
                  break;
               }

               ImGui::InputScalar("Texture Hash", ImGuiDataType_U32, &data.textureHash);

               if (ImGui::BeginCombo("Texture Addressing",
                                     data.textureAddressing == RedLightTextureAddressing::Wrap
                                        ? "Wrap"
                                        : "Clamp")) {
                  if (ImGui::Selectable("Wrap", data.textureAddressing ==
                                                   RedLightTextureAddressing::Wrap)) {
                     data.textureAddressing = RedLightTextureAddressing::Wrap;
                  }

                  if (ImGui::Selectable("Clamp", data.textureAddressing ==
                                                    RedLightTextureAddressing::Clamp)) {
                     data.textureAddressing = RedLightTextureAddressing::Clamp;
                  }

                  ImGui::EndCombo();
               }

               ImGui::ColorEdit3("Color", &data.color.r,
                                 ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);

               ImGui::TreePop();
            }

            if (light_base->RedLight_data.type == RedLightType::Directional &&
                ImGui::TreeNode("RedDirectionalLight")) {
               RedDirectionalLight_data& data =
                  ((RedDirectionalLight*)light_base)->RedDirectionalLight_data;

               ImGui::DragFloat3("Direction", &data.direction.x);

               ImGui::InputScalar("Region Name Hash", ImGuiDataType_U32,
                                  &data.regionNameHash);

               ImGui::DragFloat4("Texture Matrix 0", &data.textureMatrix.rows[0].x);
               ImGui::DragFloat4("Texture Matrix 1", &data.textureMatrix.rows[1].x);
               ImGui::DragFloat4("Texture Matrix 2", &data.textureMatrix.rows[2].x);
               ImGui::DragFloat4("Texture Matrix 3", &data.textureMatrix.rows[3].x);
               ImGui::Checkbox("Texture Matrix Dirty", &data.textureMatrixDirty);

               ImGui::DragFloat2("Texture Offset", &data.textureOffset.x);
               ImGui::DragFloat2("Texture Tiling", &data.textureTile.x);

               ImGui::TreePop();
            }

            if (light_base->RedLight_data.type == RedLightType::Omni &&
                ImGui::TreeNode("RedOmniLight")) {
               RedOmniLight_data& data = ((RedOmniLight*)light_base)->RedOmniLight_data;

               ImGui::DragFloat3("Position", &data.position.x);

               if (ImGui::DragFloat("Range", &data.range)) {
                  data.invRange = 1.0f / data.range;
                  data.invRangeSq = data.invRange * data.invRange;
               }

               ImGui::LabelText("Inv Range", "%f", data.invRange);

               ImGui::LabelText("Inv Range Sq", "%f", data.invRangeSq);

               ImGui::DragFloat4("Texture Matrix 0", &data.textureMatrix.rows[0].x);
               ImGui::DragFloat4("Texture Matrix 1", &data.textureMatrix.rows[1].x);
               ImGui::DragFloat4("Texture Matrix 2", &data.textureMatrix.rows[2].x);
               ImGui::DragFloat4("Texture Matrix 3", &data.textureMatrix.rows[3].x);
               ImGui::Checkbox("Texture Matrix Dirty", &data.textureMatrixDirty);

               ImGui::TreePop();
            }

            if (light_base->RedLight_data.type == RedLightType::Spot &&
                ImGui::TreeNode("RedSpotLight")) {
               RedSpotLight_data& data = ((RedSpotLight*)light_base)->RedSpotLight_data;

               ImGui::LabelText("Inner Angle Tan", "%f", data.innerAngleTan);
               ImGui::LabelText("Outer Angle Tan", "%f", data.outerAngleTan);
               ImGui::LabelText("Inner Angle Tan", "%f", data.innerAngleCos);
               ImGui::LabelText("Outer Angle Tan", "%f", data.outerAngleCos);

               ImGui::DragFloat("Falloff", &data.falloff);

               if (ImGui::DragFloat("Range", &data.range)) {
                  const float inv_range = 1.0f / data.range;
                  data.invRangeSq = inv_range * inv_range;
                  data.coneRadius =
                     data.range * std::tan(std::atan(data.outerAngleTan) * 0.5f);
               }

               ImGui::DragFloat3("Direction", &data.direction.x);

               ImGui::DragFloat3("Position", &data.position.x);

               ImGui::DragFloat4("Texture Matrix 0", &data.textureMatrix.rows[0].x);
               ImGui::DragFloat4("Texture Matrix 1", &data.textureMatrix.rows[1].x);
               ImGui::DragFloat4("Texture Matrix 2", &data.textureMatrix.rows[2].x);
               ImGui::DragFloat4("Texture Matrix 3", &data.textureMatrix.rows[3].x);
               ImGui::Checkbox("Texture Matrix Dirty", &data.textureMatrixDirty);

               ImGui::Checkbox("Bidirectional", &data.textureMatrixDirty);

               ImGui::LabelText("Inv Range Sq", "%f", data.invRangeSq);
               ImGui::LabelText("Cone Radius", "%f", data.coneRadius);

               ImGui::TreePop();
            }

            ImGui::TreePop();
         }

         node = node->next;
      }
   }

   ImGui::End();
}
}