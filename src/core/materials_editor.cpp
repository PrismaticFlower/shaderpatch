
#include "materials_editor.hpp"
#include "material_constant_buffers.hpp"

#include <algorithm>

#include <imgui.h>

namespace sp::core {

namespace {

void property_editor(const std::string& name, Material_var<float>& var) noexcept
{
   ImGui::InputFloat(name.c_str(), &var.value);

   var.value = std::clamp(var.value, var.min, var.max);
}

void property_editor(const std::string& name, Material_var<glm::vec2>& var) noexcept
{
   ImGui::InputFloat2(name.c_str(), &var.value.x);

   var.value = glm::clamp(var.value, var.min, var.max);
}

void property_editor(const std::string& name, Material_var<glm::vec3>& var) noexcept
{
   ImGui::InputFloat3(name.c_str(), &var.value.x);

   var.value = glm::clamp(var.value, var.min, var.max);
}

void property_editor(const std::string& name, Material_var<glm::vec4>& var) noexcept
{
   ImGui::InputFloat4(name.c_str(), &var.value.x);

   var.value = glm::clamp(var.value, var.min, var.max);
}

void property_editor(const std::string& name, Material_var<std::int32_t>& var) noexcept
{
   ImGui::InputInt(name.c_str(), &var.value);

   var.value = std::clamp(var.value, var.min, var.max);
}

void property_editor(const std::string& name, Material_var<glm::ivec2>& var) noexcept
{
   ImGui::InputInt2(name.c_str(), &var.value.x);

   var.value = glm::clamp(var.value, var.min, var.max);
}

void property_editor(const std::string& name, Material_var<glm::ivec3>& var) noexcept
{
   ImGui::InputInt3(name.c_str(), &var.value.x);

   var.value = glm::clamp(var.value, var.min, var.max);
}

void property_editor(const std::string& name, Material_var<glm::ivec4>& var) noexcept
{
   ImGui::InputInt4(name.c_str(), &var.value.x);

   var.value = glm::clamp(var.value, var.min, var.max);
}

void property_editor(const std::string& name, Material_var<std::uint32_t>& var) noexcept
{
   ImGui::InputScalar(name.c_str(), ImGuiDataType_U32, &var.value);

   var.value = std::clamp(var.value, var.min, var.max);
}

void property_editor(const std::string& name, Material_var<glm::uvec2>& var) noexcept
{
   ImGui::InputScalarN(name.c_str(), ImGuiDataType_U32, &var.value.x, 2);

   var.value = glm::clamp(var.value, var.min, var.max);
}

void property_editor(const std::string& name, Material_var<glm::uvec3>& var) noexcept
{
   ImGui::InputScalarN(name.c_str(), ImGuiDataType_U32, &var.value.x, 3);

   var.value = glm::clamp(var.value, var.min, var.max);
}

void property_editor(const std::string& name, Material_var<glm::uvec4>& var) noexcept
{
   ImGui::InputScalarN(name.c_str(), ImGuiDataType_U32, &var.value.x, 4);

   var.value = glm::clamp(var.value, var.min, var.max);
}

void property_editor(const std::string& name, Material_var<bool>& var) noexcept
{
   ImGui::Checkbox(name.c_str(), &var.value);
}

void resources_editor(Shader_resource_database& resources,
                      std::vector<Com_ptr<ID3D11ShaderResourceView>>& srvs,
                      std::vector<std::string>& srv_names)
{
   for (auto i = 0; i < srvs.size(); ++i) {
      if (!srvs[i]) continue;

      D3D11_SHADER_RESOURCE_VIEW_DESC desc{};
      srvs[i]->GetDesc(&desc);

      ImGui::PushID(i);

      if (desc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE2D ||
          desc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE2DARRAY) {
         if (ImGui::ImageButton(srvs[i].get(), {64, 64})) {
            ImGui::OpenPopup("Resource Picker");
         }
      }
      else {
         if (ImGui::Button(("Resource "s + std::to_string(i)).c_str())) {
            ImGui::OpenPopup("Resource Picker");
         }
      }

      if (ImGui::IsItemHovered()) ImGui::SetTooltip(srv_names[i].c_str());

      if (ImGui::BeginPopup("Resource Picker")) {
         auto [new_srv, name] = resources.imgui_resource_picker();

         if (new_srv) {
            srvs[i] = copy_raw_com_ptr(new_srv);
            srv_names[i] = name;
         }

         ImGui::EndPopup();
      }

      ImGui::PopID();
   }
}

void material_editor(ID3D11Device5& device, Shader_resource_database& resources,
                     Patch_material& material) noexcept
{
   if (ImGui::TreeNode("Properties")) {
      for (auto& prop : material.properties) {
         std::visit([&](auto& value) { property_editor(prop.name, value); },
                    prop.value);
      }

      material.constant_buffer =
         create_material_constant_buffer(device, material.cb_name, material.properties);

      ImGui::TreePop();
   }

   const auto create_resource_editor =
      [&resources](const char* const name,
                   std::vector<Com_ptr<ID3D11ShaderResourceView>>& srvs,
                   std::vector<std::string>& srv_names) {
         if (!srvs.empty() && ImGui::TreeNode(name)) {
            resources_editor(resources, srvs, srv_names);

            ImGui::TreePop();
         }
      };

   create_resource_editor("VS Shader Resources", material.vs_shader_resources,
                          material.vs_shader_resources_names);
   create_resource_editor("HS Shader Resources", material.hs_shader_resources,
                          material.hs_shader_resources_names);
   create_resource_editor("DS Shader Resources", material.ds_shader_resources,
                          material.ds_shader_resources_names);
   create_resource_editor("GS Shader Resources", material.gs_shader_resources,
                          material.gs_shader_resources_names);
   create_resource_editor("PS Shader Resources", material.ps_shader_resources,
                          material.ps_shader_resources_names);

   if (ImGui::TreeNode("Advanced")) {
      ImGui::Text("Rendertype: %s", material.rendertype.c_str());
      ImGui::Text("Constant Buffer Name: %s", material.cb_name.c_str());
      ImGui::Text("Overridden Rendertype: %s",
                  to_string(material.overridden_rendertype).c_str());

      ImGui::Checkbox("Tessellation", &material.tessellation);
      ImGui::Text("Tessellation Primitive Topology: %i",
                  material.tessellation_primitive_topology);

      if (ImGui::TreeNode("Fail Safe Game Texture")) {

         if (ImGui::ImageButton(material.fail_safe_game_texture.srv.get(), {64, 64})) {
            ImGui::OpenPopup("Texture Picker");
         }

         if (ImGui::BeginPopup("Texture Picker")) {
            auto [new_srv, name] = resources.imgui_resource_picker();

            if (new_srv)
               material.fail_safe_game_texture.srv = copy_raw_com_ptr(new_srv);

            ImGui::EndPopup();
         }

         material.fail_safe_game_texture.srgb_srv =
            material.fail_safe_game_texture.srv;

         ImGui::TreePop();
      }

      ImGui::TreePop();
   }
}
}

void show_materials_editor(ID3D11Device5& device, Shader_resource_database& resources,
                           const std::vector<std::unique_ptr<Patch_material>>& materials) noexcept
{
   if (ImGui::Begin("Materials")) {
      for (auto& material : materials) {
         if (ImGui::TreeNode(material->name.c_str())) {
            material_editor(device, resources, *material);
            ImGui::TreePop();
         }
      }
   }

   ImGui::End();
}
}
