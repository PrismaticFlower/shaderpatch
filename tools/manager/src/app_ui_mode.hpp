#pragma once

#include "framework.hpp"

class app_ui_mode {
public:
   virtual ~app_ui_mode() = default;

   virtual void switch_to(
      winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource xaml_source) noexcept = 0;

   virtual void switched_from() noexcept = 0;

   virtual void update(MSG& msg) noexcept = 0;
};
