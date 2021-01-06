
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
         ImGui::InputText(key.c_str(), &value);
      }

      ImGui::TreePop();
   }

   if (ImGui::TreeNode("Advanced")) {
      ImGui::Text("Rendertype: %s", material.rendertype.c_str());
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
