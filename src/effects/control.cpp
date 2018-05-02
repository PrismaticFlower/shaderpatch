
#include "control.hpp"
#include "../imgui/imgui.h"

namespace sp::effects {

void Control::show_imgui() noexcept
{
   ImGui::Begin("Effects Control", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

   ImGui::Checkbox("Enable Effects", &_enabled);

   if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Device must be reset before this will take efect.");
   }

   ImGui::End();
}

}
