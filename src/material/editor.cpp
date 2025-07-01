
#include "editor.hpp"

#include <algorithm>
#include <type_traits>

#include "../imgui/imgui.h"
#include "../imgui/imgui_stdlib.h"

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
constexpr auto property_speed() noexcept -> float
{
   if constexpr (std::is_integral_v<Type>) return 0.25f;
   if constexpr (std::is_floating_point_v<Type>) return 0.01f;
}

template<typename Type>
struct Property_traits {
   inline constexpr static ImGuiDataType data_type = property_datatype<Type>();
   inline constexpr static float speed = property_speed<Type>();
   inline constexpr static int length = 1;
};

template<typename Type, std::size_t len>
struct Property_traits<glm::vec<len, Type>> {
   inline constexpr static ImGuiDataType data_type = property_datatype<Type>();
   inline constexpr static float speed = property_speed<Type>();
   inline constexpr static int length = len;
};

constexpr auto v = Property_traits<float>::length;

template<typename Var_type>
void property_editor(const std::string& name, Material_var<Var_type>& var) noexcept
{
   using Traits = Property_traits<Var_type>;

   ImGui::DragScalarN(name.c_str(), Traits::data_type, &var.value,
                      Traits::length, Traits::speed, &var.min, &var.max);

   var.value = glm::clamp(var.value, var.min, var.max);
}

void property_editor(const std::string& name, Material_var<glm::vec3>& var) noexcept
{
   if (name.find("Color") != name.npos) {
      ImGui::ColorEdit3(name.c_str(), &var.value[0], ImGuiColorEditFlags_Float);
   }
   else {
      using Traits = Property_traits<glm::vec3>;

      ImGui::DragScalarN(name.c_str(), Traits::data_type, &var.value,
                         Traits::length, Traits::speed, &var.min, &var.max);
   }

   var.value = glm::clamp(var.value, var.min, var.max);
}

void property_editor(const std::string& name, Material_var<bool>& var) noexcept
{
   ImGui::Checkbox(name.c_str(), &var.value);
}

void material_editor(Factory& factory, Material& material) noexcept
{
   if (!material.properties.empty() && ImGui::TreeNode("Properties")) {
      for (auto& prop : material.properties) {
         std::visit([&](auto& value) { property_editor(prop.name, value); },
                    prop.value);
      }

      ImGui::TreePop();
   }

   if (!material.resource_properties.empty() &&
       ImGui::TreeNode("Shader Resources")) {
      for (auto& [key, value] : material.resource_properties) {
         if (ImGui::BeginCombo(key.c_str(), value.data(), ImGuiComboFlags_HeightLargest)) {
            const core::Shader_resource_database::Imgui_pick_result picked =
               factory.shader_resource_database().imgui_resource_picker();

            if (picked.srv) {
               value = picked.name;
            }

            ImGui::EndCombo();
         }
      }

      ImGui::TreePop();
   }

   if (ImGui::TreeNode("Advanced")) {
      ImGui::Text("Type: %s", material.type.c_str());
      ImGui::Text("Overridden Rendertype: %s",
                  to_string(material.overridden_rendertype).c_str());

      ImGui::TreePop();
   }

   factory.update_material(material);
}
}

void show_editor(Factory& factory,
                 const std::vector<std::unique_ptr<Material>>& materials) noexcept
{
   if (ImGui::Begin("Materials")) {
      for (auto& material : materials) {
         if (ImGui::TreeNode(material->name.c_str())) {
            material_editor(factory, *material);
            ImGui::TreePop();
         }
      }
   }

   ImGui::End();
}
}
