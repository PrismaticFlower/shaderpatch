#include "framework.hpp"

#include "app_ui_mode.hpp"
#include "config_ui.hpp"
#include "overloaded.hpp"
#include "user_config.hpp"
#include "user_config_descriptions.hpp"
#include "user_config_loader.hpp"
#include "user_config_saver.hpp"
#include "xaml_ui_helpers.hpp"

using namespace std::literals;
using namespace winrt::Windows::UI;

namespace {

auto make_config_ui(std::shared_ptr<user_config> config) -> config::ui_root;

}

class app_mode_configurator final : public app_ui_mode {
public:
   app_mode_configurator(Xaml::Hosting::DesktopWindowXamlSource xaml_source,
                         const bool display_installed_dialog) noexcept
   {
      navigation_view = config::create_xaml_ui_element(config_ui, config_changed);

      create_about_page();

      root_xaml_element.Children().Append(navigation_view);

      xaml_source.Content(root_xaml_element);

      if (display_installed_dialog) {
         Xaml::Controls::ContentDialog installed_dialog;
         installed_dialog.Title(winrt::box_value(L"Shader Patch Installed"sv));
         installed_dialog.PrimaryButtonText(L"Okay"sv);
         installed_dialog.DefaultButton(Xaml::Controls::ContentDialogButton::Primary);
         installed_dialog.Content(winrt::box_value(
            L"Installation successful! To uninstall go into the About section in the configurator."sv));

         root_xaml_element.Children().Append(installed_dialog);

         installed_dialog.ShowAsync();
      }

      uninstall_failed_dialog.Title(winrt::box_value(L"Uninstall Failed!"sv));
      uninstall_failed_dialog.PrimaryButtonText(L"Okay"sv);
      uninstall_failed_dialog.DefaultButton(Xaml::Controls::ContentDialogButton::Primary);
      uninstall_failed_dialog.Content(winrt::box_value(
         L"Failed to launch PowerShell to uninstall Shader Patch!"sv));

      root_xaml_element.Children().Append(uninstall_failed_dialog);
   }

   auto update([[maybe_unused]] MSG& msg) noexcept -> app_update_result override
   {
      if (std::exchange(*config_changed, false)) {
         user_config_saver.enqueue_async_save(*config);
      }

      return app_update_result::none;
   }

private:
   void create_about_page() noexcept
   {
      Xaml::Controls::ScrollViewer about_scroll_viewer;
      about_scroll_viewer.Padding(Xaml::Thickness{0, 8, 0, 0});
      about_scroll_viewer.Name(L"About"sv);
      about_scroll_viewer.Visibility(Xaml::Visibility::Collapsed);

      Xaml::Controls::StackPanel about_panel;
      about_panel.Padding(Xaml::Thickness{16, 0, 0, 0});

      Xaml::Controls::StackPanel links_panel;
      {
         Xaml::Controls::TextBlock title;
         title.Text(L"Links"sv);
         apply_text_style(title, text_style::title);

         links_panel.Children().Append(title);

         for (auto [link, text] :
              {std::pair{L"https://github.com/SleepKiller/shaderpatch/releases/latest"sv,
                         L"Latest Release"sv},
               std::pair{L"https://github.com/SleepKiller/shaderpatch/issues"sv,
                         L"Known Issues"sv},
               std::pair{L"https://github.com/SleepKiller/shaderpatch"sv,
                         L"GitHub Repository"sv}}) {
            Xaml::Controls::HyperlinkButton button;
            button.NavigateUri(winrt::Windows::Foundation::Uri{link});
            button.Content(winrt::box_value(text));

            links_panel.Children().Append(button);
         }
      }

      about_panel.Children().Append(links_panel);

      Xaml::Controls::StackPanel uninstall_panel;
      {
         Xaml::Controls::TextBlock title;
         title.Text(L"Uninstall"sv);
         apply_text_style(title, text_style::title);

         uninstall_panel.Children().Append(title);

         Xaml::Controls::StackPanel contents_panel;
         contents_panel.Orientation(Xaml::Controls::Orientation::Horizontal);
         contents_panel.Padding(Xaml::Thickness{0, 8, 0, 0});
         {
            Xaml::Controls::StackPanel button_panel;
            button_panel.Orientation(Xaml::Controls::Orientation::Horizontal);
            {
               Xaml::Controls::SymbolIcon icon;
               icon.Symbol(Xaml::Controls::Symbol::Delete);

               button_panel.Children().Append(icon);

               Xaml::Controls::TextBlock button_text;
               button_text.Text(L"Uninstall"sv);
               button_text.Margin(Xaml::Thickness{4, 0, 0, 0});

               button_panel.Children().Append(button_text);
            }

            Xaml::Controls::Button button;
            button.HorizontalAlignment(Xaml::HorizontalAlignment::Center);
            button.VerticalAlignment(Xaml::VerticalAlignment::Center);
            button.Content(button_panel);
            button.IsEnabled(uninstallable);
            button.Click({this, &app_mode_configurator::uninstall_clicked});

            contents_panel.Children().Append(button);

            Xaml::Controls::TextBlock button_desc;
            button_desc.HorizontalAlignment(Xaml::HorizontalAlignment::Center);
            button_desc.VerticalAlignment(Xaml::VerticalAlignment::Center);
            button_desc.Margin(Xaml::Thickness{8, 0, 0, 0});
            button_desc.IsTextSelectionEnabled(true);

            if (uninstallable) {
               button_desc.Text(
                  L"Exit the app and launch a script to remove all the files "
                  L"copied over when installing Shader Patch.");
            }
            else {
               button_desc.Text(
                  L"Automatic Uninstall is unavailable. The install manifest "
                  L"is missing. This likely means you manually installed Shader Patch."sv);
            }

            contents_panel.Children().Append(button_desc);
         }

         uninstall_panel.Children().Append(contents_panel);
      }

      about_panel.Children().Append(uninstall_panel);

      about_scroll_viewer.Content(about_panel);

      navigation_view.Content().as<Xaml::Controls::Panel>().Children().Append(
         about_scroll_viewer);
      navigation_view.MenuItems().Append(winrt::box_value(L"About"sv));
   }

   void uninstall_clicked([[maybe_unused]] winrt::Windows::Foundation::IInspectable sender,
                          [[maybe_unused]] Xaml::RoutedEventArgs args)
   {
      std::wstring commandline =
         LR"(powershell.exe )"s + LR"(")" + LR"(Wait-Process -Id )"s +
         std::to_wstring(GetCurrentProcessId()) + L"; "s +
         LR"((Get-Content data\shaderpatch\install.manifest) | ForEach { Remove-Item -Force -Path $_ })"s +
         LR"(")";

      STARTUPINFOW startup_info{};
      startup_info.cb = sizeof(STARTUPINFOW);
      PROCESS_INFORMATION info{};

      if (CreateProcessW(nullptr, commandline.data(), nullptr, nullptr, false,
                         CREATE_NO_WINDOW, nullptr, nullptr, &startup_info, &info)) {
         CloseHandle(info.hThread);
         CloseHandle(info.hProcess);

         std::exit(0);
      }
      else {
         uninstall_failed_dialog.ShowAsync();
      }
   }

   std::shared_ptr<user_config> config =
      std::make_shared<user_config>(load_user_config(L"shader patch.yml"sv));
   config::ui_root config_ui = make_config_ui(config);
   std::shared_ptr<bool> config_changed = std::make_shared<bool>();
   user_config_saver user_config_saver;

   Xaml::Controls::Grid root_xaml_element;
   Xaml::Controls::NavigationView navigation_view;
   Xaml::Controls::ContentDialog uninstall_failed_dialog;

   const bool uninstallable =
      std::filesystem::exists(LR"(data\shaderpatch\install.manifest)"sv);
};

auto make_app_mode_configurator(Xaml::Hosting::DesktopWindowXamlSource xaml_source,
                                const bool display_installed_dialog)
   -> std::unique_ptr<app_ui_mode>
{
   return std::make_unique<app_mode_configurator>(xaml_source, display_installed_dialog);
}

namespace {

auto make_config_ui(std::shared_ptr<user_config> config) -> config::ui_root
{
   config::ui_root root;

   const auto make_page = [&](std::wstring_view name,
                              user_config_value_vector& user_config_settings) {
      auto& page = root.pages.emplace_back();

      page.name = name;
      page.description =
         winrt::to_hstring(user_config_descriptions.at(winrt::to_string(name)));

      auto& elements = page.elements;

      for (auto& setting : user_config_settings) {
         const auto init_common = [config](auto& control, auto& setting) {
            control.header = setting.name;
            control.description = winrt::to_hstring(
               user_config_descriptions.at(winrt::to_string(setting.name)));
            control.value =
               std::shared_ptr<typename std::remove_reference_t<decltype(setting)>::value_type>{
                  config, &setting.value};
         };

         std::visit(
            overloaded{[&](bool_user_config_value& setting) {
                          auto& control =
                             elements.emplace_back().emplace<config::toggle_switch>();

                          init_common(control, setting);

                          control.on_string = setting.on_display;
                          control.off_string = setting.off_display;
                       },

                       [&](percentage_user_config_value& setting) {
                          auto& control =
                             elements.emplace_back().emplace<config::uint_slider>();

                          init_common(control, setting);

                          control.min = setting.min_value;
                          control.max = 100;
                       },

                       [&](uint2_user_config_value& setting) {
                          auto& control =
                             elements.emplace_back().emplace<config::uint2_boxes>();

                          init_common(control, setting);

                          control.subheaders = setting.value_names;
                       },

                       [&](enum_user_config_value& setting) {
                          auto& control =
                             elements.emplace_back().emplace<config::combo_box>();

                          init_common(control, setting);

                          control.items = setting.possible_values;
                       },

                       [&](string_user_config_value& setting) {
                          auto& control =
                             elements.emplace_back().emplace<config::text_box>();

                          init_common(control, setting);
                       },

                       [&](color_user_config_value& setting) {
                          auto& control =
                             elements.emplace_back().emplace<config::color_picker>();

                          init_common(control, setting);
                       }},

            setting);
      }
   };

   make_page(L"Display"sv, config->display);
   make_page(L"User Interface"sv, config->user_interface);
   make_page(L"Graphics"sv, config->graphics);
   make_page(L"Effects"sv, config->effects);
   make_page(L"Developer"sv, config->developer);

   root.shader_patch_enabled_value =
      std::shared_ptr<bool>{config, &config->enabled.value};

   return root;
}

}
