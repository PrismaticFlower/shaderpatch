#pragma once

#include "framework.hpp"

#include "app_ui_mode.hpp"

auto make_app_mode_configurator(winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource xaml_source,
                                const bool display_installed_dialog = false)
   -> std::unique_ptr<app_ui_mode>;
