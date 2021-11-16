#pragma once

#include "framework.hpp"

namespace config {

struct ui_element_base {
   winrt::hstring header;
   winrt::hstring description;
};

struct toggle_switch : ui_element_base {
   winrt::hstring on_string;
   winrt::hstring off_string;
   std::shared_ptr<bool> value;
};

struct uint_slider : ui_element_base {
   std::uint32_t min = 0;
   std::uint32_t max = 100;
   std::shared_ptr<std::uint32_t> value;
};

struct uint2_boxes : ui_element_base {
   std::array<winrt::hstring, 2> subheaders;
   std::shared_ptr<std::array<std::uint32_t, 2>> value;
};

struct combo_box : ui_element_base {
   std::vector<winrt::hstring> items;
   std::shared_ptr<winrt::hstring> value;
};

struct text_box : ui_element_base {
   std::shared_ptr<winrt::hstring> value;
};

struct color_picker : ui_element_base {
   std::shared_ptr<std::array<std::uint8_t, 3>> value;
};

using ui_element =
   std::variant<toggle_switch, uint_slider, uint2_boxes, combo_box, text_box, color_picker>;

struct ui_page {
   winrt::hstring name;
   winrt::hstring description;
   std::vector<ui_element> elements;
};

struct ui_root {
   std::vector<ui_page> pages;
   std::shared_ptr<bool> shader_patch_enabled_value;
};

auto create_xaml_ui_element(const ui_root& root, std::shared_ptr<bool> value_changed)
   -> winrt::Windows::UI::Xaml::Controls::NavigationView;

}
