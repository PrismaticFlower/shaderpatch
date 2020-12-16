#pragma once

#include "framework.hpp"

#include "app_ui_mode.hpp"

auto make_app_mode_installer(winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource xaml_source)
   -> std::unique_ptr<app_ui_mode>;
