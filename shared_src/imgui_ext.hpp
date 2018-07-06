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
   string.resize(max_length);

   thread_local std::array<char, max_length> buffer{};
   *std::copy(std::cbegin(string), std::cend(string), std::begin(buffer)) = '\0';

   bool result =
      InputText(label, buffer.data(), buffer.size(), flags, callback, user_data);

   string = buffer.data();

   return result;
}

}
