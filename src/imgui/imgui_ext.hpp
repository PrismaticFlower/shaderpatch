#pragma once

#include "imgui.h"

#include <algorithm>
#include <array>
#include <string>

#include <gsl/gsl>

namespace ImGui {

inline bool InputText(const char* label, std::string& string,
                      ImGuiInputTextFlags flags = 0,
                      ImGuiInputTextCallback callback = nullptr,
                      void* user_data = nullptr)
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

inline bool DragFloatFormattedN(const char* label, float* v, int components,
                                float v_speed, const float* v_min,
                                const float* v_max, const char** formats)
{
   bool value_changed = false;

   BeginGroup();
   PushID(label);
   PushItemWidth(CalcItemWidth());

   for (int i = 0; i < components; i++) {
      PushID(i);
      value_changed |= DragScalar("##v", ImGuiDataType_Float, &v[i], v_speed,
                                  v_min, v_max, formats[i]);
      SameLine(0, GetStyle().ItemInnerSpacing.x);
      PopID();
      PopItemWidth();
   }
   PopID();

   TextUnformatted(label);
   EndGroup();

   return value_changed;
}

inline bool DragFloatFormatted2(const char* label, float v[2],
                                std::array<const char*, 2> formats,
                                float v_speed = 1.0f, float v_min = 0.0f,
                                float v_max = 0.0f)
{
   return DragFloatFormattedN(label, v, 2, v_speed, &v_min, &v_max, formats.data());
}

inline bool DragFloatFormatted3(const char* label, float v[3],
                                std::array<const char*, 3> formats,
                                float v_speed = 1.0f, float v_min = 0.0f,
                                float v_max = 0.0f)
{
   return DragFloatFormattedN(label, v, 3, v_speed, &v_min, &v_max, formats.data());
}

inline bool DragFloatFormatted4(const char* label, float v[4],
                                std::array<const char*, 4> formats,
                                float v_speed = 1.0f, float v_min = 0.0f,
                                float v_max = 0.0f)
{
   return DragFloatFormattedN(label, v, 4, v_speed, &v_min, &v_max, formats.data());
}
}
