#pragma once

#include <imgui.h>

#include <algorithm>
#include <array>
#include <string>

#include <gsl/gsl>

namespace ImGui {

inline bool InputText(const char* label, std::string& string,
                      ImGuiInputTextFlags flags = 0,
                      ImGuiTextEditCallback callback = nullptr, void* user_data = nullptr)
{
   constexpr auto max_length = 4096;

   Expects(string.size() < max_length);

   thread_local std::array<char, max_length> buffer{};
   *std::copy(std::cbegin(string), std::cend(string), std::begin(buffer)) = '\0';

   bool result =
      InputText(label, buffer.data(), buffer.size(), flags, callback, user_data);

   string = buffer.data();

   return result;
}

template<typename Strings>
inline auto StringPicker(const char* label, const std::string& current, Strings strings)
   -> std::string
{
   std::string selected_str = current;

   if (BeginCombo(label, current.c_str(), ImGuiComboFlags_HeightRegular)) {
      for (const auto& str : strings) {
         bool selected = str == current;

         if (Selectable(str.c_str(), &selected)) selected_str = str;
         if (selected) SetItemDefaultFocus();
      }

      EndCombo();
   }

   return selected_str;
}

}
