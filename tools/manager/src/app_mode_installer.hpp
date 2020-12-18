#pragma once

#include "framework.hpp"

#include "app_ui_mode.hpp"

auto make_app_mode_installer(winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource xaml_source)
   -> std::unique_ptr<app_ui_mode>;

auto make_app_mode_installer_with_selected_path(
   winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource xaml_source,
   std::filesystem::path path) -> std::unique_ptr<app_ui_mode>;
