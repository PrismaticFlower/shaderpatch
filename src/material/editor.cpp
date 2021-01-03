
#include "editor.hpp"

#include <algorithm>
#include <type_traits>

#include "../imgui/imgui.h"

using namespace std::literals;

namespace sp::material {

namespace {

template<typename Type>
constexpr auto property_datatype() noexcept -> ImGuiDataType
{
   if constexpr (std::is_same_v<Type, glm::uint8>) return ImGuiDataType_U8;
   if constexpr (std::is_same_v<Type, glm::uint16>) return ImGuiDataType_U16;
   if constexpr (std::is_same_v<Type, glm::uint32>) return ImGuiDataType_U32;
   if constexpr (std::is_same_v<Type, glm::uint64>) return ImGuiDataType_U64;
   if constexpr (std::is_same_v<Type, glm::int8>) return ImGuiDataType_S8;
   if constexpr (std::is_same_v<Type, glm::int16>) return ImGuiDataType_S16;
   if constexpr (std::is_same_v<Type, glm::int32>) return ImGuiDataType_S32;
   if constexpr (std::is_same_v<Type, glm::int64>) return ImGuiDataType_S64;
   if constexpr (std::is_same_v<Type, float>) return ImGuiDataType_Float;
   if constexpr (std::is_same_v<Type, float>) return ImGuiDataType_Double;
}

template<typename Type>
struct Property_traits {
   inline constexpr static auto data_type = property_datatype<Type>();
   inline constexpr static auto length = 1;
};

template<typename Type, std::size_t len>
struct Property_traits<glm::vec<len, Type>> {
   inline constexpr static auto data_type = property_datatype<Type>();
   inline constexpr static auto length = len;
};

constexpr auto v = Property_traits<float>::length;

template<typename Var_type>
void property_editor(const std::string& name, Material_var<Var_type>& var) noexcept
{
   ImGui::DragScalarN(name.c_str(), Property_traits<Var_type>::data_type, &var.value,
                      Property_traits<Var_type>::length, 0.01f, &var.min, &var.max);

   var.value = glm::clamp(var.value, var.min, var.max);
}

void property_editor(const std::string& name, Material_var<bool>& var) noexcept
{
   ImGui::Checkbox(name.c_str(), &var.value);
}

void resources_editor(core::Shader_resource_database& resources,
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

void material_editor(Factory& factory, core::Shader_resource_database& resources,
                     Material& material) noexcept
{
   if (!material.properties.empty() && ImGui::TreeNode("Properties")) {
      for (auto& prop : material.properties) {
         std::visit([&](auto& value) { property_editor(prop.name, value); },
                    prop.value);
      }

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
   create_resource_editor("PS Shader Resources", material.ps_shader_resources,
                          material.ps_shader_resources_names);

   if (ImGui::TreeNode("Advanced")) {
      ImGui::Text("Rendertype: %s", material.rendertype.c_str());
      ImGui::Text("Constant Buffer Name: %s", material.cb_name.c_str());
      ImGui::Text("Overridden Rendertype: %s",
                  to_string(material.overridden_rendertype).c_str());

      if (ImGui::TreeNode("Fail Safe Game Texture")) {

         if (ImGui::ImageButton(material.fail_safe_game_texture.get(), {64, 64})) {
            ImGui::OpenPopup("Texture Picker");
         }

         if (ImGui::BeginPopup("Texture Picker")) {
            auto [new_srv, name] = resources.imgui_resource_picker();

            if (new_srv)
               material.fail_safe_game_texture = copy_raw_com_ptr(new_srv);

            ImGui::EndPopup();
         }

         ImGui::TreePop();
      }

      ImGui::TreePop();
   }

   factory.update_material(material);
}
}

void show_editor(Factory& factory, core::Shader_resource_database& resources,
                 const std::vector<std::unique_ptr<Material>>& materials) noexcept
{
   if (ImGui::Begin("Materials")) {
      for (auto& material : materials) {
         if (ImGui::TreeNode(material->name.c_str())) {
            material_editor(factory, resources, *material);
            ImGui::TreePop();
         }
      }
   }

   ImGui::End();
}
}
