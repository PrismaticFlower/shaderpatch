#include "framework.hpp"

#include "app_mode_installer.hpp"
#include "game_locator.hpp"
#include "installer.hpp"
#include "open_file_dialog.hpp"
#include "xaml_ui_helpers.hpp"

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::UI;

namespace {

class app_mode_installer : public app_ui_mode {
public:
   app_mode_installer(Windows::UI::Xaml::Hosting::DesktopWindowXamlSource xaml_source)
   {
      create_controls();

      for (auto path : search_registry_locations_for_game_installs()) {
         install_locations.Append(winrt::box_value(path.native()));
      }

      xaml_source.as<IDesktopWindowXamlSourceNative2>()->get_WindowHandle(&window);
      xaml_source.Content(ui_root);
   }

   auto update([[maybe_unused]] MSG& msg) noexcept -> app_update_result override
   {
      if (game_disk_searcher) {
         game_disk_searcher->get_discovered([this](std::filesystem::path path) {
            if (std::find_if(begin(install_locations), end(install_locations),
                             [&](const IInspectable& entry) {
                                return std::filesystem::equivalent(
                                   std::wstring{unbox_value<hstring>(entry)}, path);
                             }) != end(install_locations)) {
               return;
            }

            install_locations.Append(box_value(path.native()));
         });

         if (game_disk_searcher->completed()) {
            game_disk_searcher = std::nullopt;
            searching_for_installs_status.Visibility(Xaml::Visibility::Collapsed);
         }
      }

      if (installer) {
         switch (installer->status()) {
         case installer_status::inprogress: {
            installing_status.Text(installer->status_message());
            installing_progress_bar.Value(installer->progress());
            break;
         }
         case installer_status::completed: {
            installer = std::nullopt;

            std::filesystem::current_path(install_path);

            return app_update_result::switch_to_configurator;
         }
         case installer_status::permission_denied: {
            installer = std::nullopt;

            change_active_page(permission_required_page);
            break;
         }
         case installer_status::failure: {
            for (const auto& path : installer->failed_to_remove_files()) {
               installation_failed_remaining_files.Append(box_value(path.native()));
            }

            installer = std::nullopt;

            change_active_page(installation_failed_page);
            break;
         }
         }
      }

      return app_update_result::none;
   }

private:
   void create_controls() noexcept
   {
      using namespace Xaml;
      using namespace Xaml::Controls;

      create_selection_page();
      create_installing_page();
      create_permission_required_page();
      create_installation_failed_page();

      ui_root.Children().Append(selection_page);
      ui_root.Children().Append(installing_page);
      ui_root.Children().Append(permission_required_page);
      ui_root.Children().Append(installation_failed_page);
   }

   void create_selection_page() noexcept
   {
      using namespace Xaml;
      using namespace Xaml::Controls;

      TextBlock header;
      header.Text(L"Install");
      header.Padding(Thickness{8.0, 0.0, 0.0, 0.0});
      apply_text_style(header, text_style::header);

      selection_page.Children().Append(header);

      TextBlock title;
      title.Text(L"Select your install location:");
      title.Padding(Thickness{16.0, 0.0, 0.0, 8.0});
      apply_text_style(title, text_style::title);

      selection_page.Children().Append(title);

      StackPanel app_bar;
      app_bar.Orientation(Orientation::Horizontal);
      {
         SymbolIcon browse_icon;
         browse_icon.Symbol(Symbol::Folder);

         AppBarButton browse;
         browse.Icon(browse_icon);
         browse.Label(L"Browse");
         browse.Click({this, &app_mode_installer::browse_for_installs_clicked});
         set_tooltip(browse, L"Browse for your game install manually.");

         app_bar.Children().Append(browse);

         SymbolIcon find_icon;
         find_icon.Symbol(Symbol::Find);

         AppBarButton auto_locate;
         auto_locate.Icon(find_icon);
         auto_locate.Label(L"Auto Locate");
         auto_locate.Click({this, &app_mode_installer::auto_locate_installs_clicked});
         set_tooltip(auto_locate, L"Have the installer try and locate your "
                                  L"game install automatically.");

         app_bar.Children().Append(auto_locate);
      }

      selection_page.Children().Append(app_bar);

      ListView install_location_picker;
      install_location_picker.Padding(Thickness{16, 0, 0, 0});
      install_location_picker.IsItemClickEnabled(true);
      install_location_picker.ItemClick({this, &app_mode_installer::location_selected});
      install_locations = install_location_picker.Items();

      selection_page.Children().Append(install_location_picker);

      searching_for_installs_status.Orientation(Orientation::Horizontal);
      searching_for_installs_status.HorizontalAlignment(HorizontalAlignment::Center);
      searching_for_installs_status.Visibility(Visibility::Collapsed);
      {

         TextBlock label;
         label.Text(L"Searching for game installs...");
         label.Padding(Thickness{16.0, 0.0, 8.0, 0.0});
         label.VerticalAlignment(VerticalAlignment::Center);
         label.TextAlignment(TextAlignment::Center);

         searching_for_installs_status.Children().Append(label);

         ProgressRing ring;
         ring.IsActive(true);
         ring.HorizontalAlignment(HorizontalAlignment::Center);

         searching_for_installs_status.Children().Append(ring);
      }

      selection_page.Children().Append(searching_for_installs_status);

      TextBlock manual_title;
      manual_title.Text(L"Manual Install");
      manual_title.Padding(Thickness{16.0, 32.0, 0.0, 0.0});
      apply_text_style(manual_title, text_style::subtitle);

      selection_page.Children().Append(manual_title);

      TextBlock manual_description;
      manual_description.Text(
         L"Prefer not to use the installer? Shader Patch is fully portable and "
         L"can simply be copied into your GameData folder by hand. Once done "
         L"it will be installed and \"just work\". This app will even start "
         L"functioning as a configurator for it when ran from the GameData "
         L"folder.");
      manual_description.Padding(Thickness{16.0, 2.0, 0.0, 0.0});
      manual_description.TextWrapping(TextWrapping::WrapWholeWords);
      manual_description.IsTextSelectionEnabled(true);
      manual_description.MaxWidth(500.0);
      manual_description.HorizontalAlignment(HorizontalAlignment::Left);

      selection_page.Children().Append(manual_description);
   }

   void create_installing_page() noexcept
   {
      using namespace Xaml;
      using namespace Xaml::Controls;

      installing_page.Visibility(Visibility::Collapsed);

      TextBlock header;
      header.Text(L"Installing...");
      header.Padding(Thickness{8.0, 0.0, 0.0, 0.0});
      apply_text_style(header, text_style::header);

      installing_page.Children().Append(header);

      StackPanel status_panel;
      status_panel.HorizontalAlignment(HorizontalAlignment::Center);
      status_panel.VerticalAlignment(VerticalAlignment::Center);
      {
         installing_status.Text(L"Preparing to install.");
         installing_status.HorizontalAlignment(HorizontalAlignment::Center);
         installing_status.VerticalAlignment(VerticalAlignment::Center);
         apply_text_style(installing_status, text_style::title);

         status_panel.Children().Append(installing_status);

         installing_progress_bar.Width(256.0);
         installing_progress_bar.Height(16.0);
         installing_progress_bar.HorizontalAlignment(HorizontalAlignment::Center);
         installing_progress_bar.VerticalAlignment(VerticalAlignment::Center);
         installing_progress_bar.Margin(Thickness{0, 16, 0, 0});

         status_panel.Children().Append(installing_progress_bar);
      }

      installing_page.Children().Append(status_panel);
   }

   void create_permission_required_page() noexcept
   {
      using namespace Xaml;
      using namespace Xaml::Controls;

      permission_required_page.Visibility(Visibility::Collapsed);

      TextBlock header;
      header.Text(L"Permission Required");
      header.Padding(Thickness{8.0, 0.0, 0.0, 0.0});
      apply_text_style(header, text_style::header);

      permission_required_page.Children().Append(header);

      StackPanel content_panel;
      content_panel.HorizontalAlignment(HorizontalAlignment::Center);
      content_panel.VerticalAlignment(VerticalAlignment::Center);
      {
         TextBlock title;

         title.Text(L"File Write Permission Required.");
         title.HorizontalAlignment(HorizontalAlignment::Center);
         title.VerticalAlignment(VerticalAlignment::Center);
         apply_text_style(title, text_style::title);

         content_panel.Children().Append(title);

         TextBlock explanation;

         explanation.Text(
            L"Your user account does not have write access to one or more "
            L"items in the game's directory. You can Retry with Admin "
            L"Permissions or you can Cancel and correct the file permissions "
            L"manually before then retrying as a regular user.");
         explanation.TextWrapping(TextWrapping::WrapWholeWords);
         explanation.IsTextSelectionEnabled(true);
         explanation.Padding(Thickness{0, 8, 0, 0});
         explanation.MaxWidth(600.0);

         content_panel.Children().Append(explanation);

         StackPanel buttons_panel;
         buttons_panel.Orientation(Orientation::Horizontal);
         buttons_panel.HorizontalAlignment(HorizontalAlignment::Center);
         buttons_panel.Padding(Thickness{0, 8, 0, 0});
         {
            const auto make_button = [](IconElement icon, winrt::hstring text,
                                        Thickness margin,
                                        RoutedEventHandler click_handler) {
               Button button;
               button.Margin(margin);

               StackPanel panel;
               panel.Orientation(Orientation::Horizontal);

               TextBlock text_block;
               text_block.Margin(Thickness{4, 0, 0, 0});
               text_block.Text(text);

               panel.Children().Append(icon);
               panel.Children().Append(text_block);

               button.Content(panel);
               button.Click(click_handler);

               return button;
            };

            Media::FontFamily font{L"Segoe MDL2 Assets"};

            FontIcon shield_icon;
            shield_icon.Glyph(L"\uea18");
            shield_icon.FontFamily(font);

            SymbolIcon cancel_icon;
            cancel_icon.Symbol(Symbol::Cancel);

            buttons_panel.Children().Append(
               make_button(shield_icon, L"Retry with Admin Permissions",
                           Thickness{0, 0, 0, 0},
                           {this, &app_mode_installer::retry_install_with_admin}));
            buttons_panel.Children().Append(
               make_button(cancel_icon, L"Cancel", Thickness{8, 0, 0, 0},
                           {this, &app_mode_installer::cancel_install}));
         }

         content_panel.Children().Append(buttons_panel);
      }

      permission_required_page.Children().Append(content_panel);
   }

   void create_installation_failed_page() noexcept
   {
      using namespace Xaml;
      using namespace Xaml::Controls;

      installation_failed_page.Visibility(Visibility::Collapsed);

      TextBlock header;
      header.Text(L"Installation Failed");
      header.Padding(Thickness{8.0, 0.0, 0.0, 0.0});
      apply_text_style(header, text_style::header);

      installation_failed_page.Children().Append(header);

      StackPanel info_panel;
      info_panel.HorizontalAlignment(HorizontalAlignment::Center);
      info_panel.VerticalAlignment(VerticalAlignment::Center);
      {
         TextBlock title;
         title.Text(L"Installation Failed");
         apply_text_style(title, text_style::title);

         info_panel.Children().Append(title);

         TextBlock message;
         message.Text(L"An unexpected error occured while installing! The "
                      L"application has attempted to remove all files copied "
                      L"before failure. Any files that were copied before "
                      L"failure but could not be removed are listed below.");
         message.TextWrapping(TextWrapping::WrapWholeWords);
         message.IsTextSelectionEnabled(true);
         message.Padding(Thickness{0, 8, 0, 0});
         message.MaxWidth(600);

         info_panel.Children().Append(message);

         TextBlock remaining_header;
         remaining_header.Text(L"Remaining Files");
         remaining_header.Margin(Thickness{4, 32, 0, 0});
         apply_text_style(remaining_header, text_style::subtitle);

         info_panel.Children().Append(remaining_header);

         ListBox remaining_list;
         remaining_list.MaxWidth(600.0);
         remaining_list.MaxHeight(256.0);
         remaining_list.Margin(Thickness{0, 4, 0, 0});

         installation_failed_remaining_files = remaining_list.Items();
         info_panel.Children().Append(remaining_list);
      }

      installation_failed_page.Children().Append(info_panel);
   }

   void change_active_page(Xaml::UIElement new_page)
   {
      for (auto child : ui_root.Children()) {
         child.Visibility(Xaml::Visibility::Collapsed);
      }

      new_page.Visibility(Xaml::Visibility::Visible);
   }

   void browse_for_installs_clicked([[maybe_unused]] IInspectable sender,
                                    [[maybe_unused]] Xaml::RoutedEventArgs args) noexcept
   {
      if (auto path =
             open_file_dialog({L"SWBFII Executable", L"BattlefrontII.exe"});
          path) {
         install_locations.Append(box_value(path->native()));
      }
   }

   void auto_locate_installs_clicked(IInspectable sender,
                                     [[maybe_unused]] Xaml::RoutedEventArgs args) noexcept
   {
      game_disk_searcher.emplace();
      searching_for_installs_status.Visibility(Xaml::Visibility::Visible);
      sender.as<Xaml::Controls::Control>().IsEnabled(false);
   }

   void location_selected([[maybe_unused]] IInspectable sender,
                          Xaml::Controls::ItemClickEventArgs args) noexcept
   {
      game_disk_searcher = std::nullopt;

      change_active_page(installing_page);

      std::filesystem::path selected_exe_path =
         std::wstring{unbox_value<hstring>(args.ClickedItem())};
      install_path = selected_exe_path.parent_path();

      installer.emplace(install_path);
   }

   void retry_install_with_admin([[maybe_unused]] IInspectable sender,
                                 [[maybe_unused]] Xaml::RoutedEventArgs args) noexcept
   {
      restart_and_install_as_admin(install_path);
   }

   void cancel_install([[maybe_unused]] IInspectable sender,
                       [[maybe_unused]] Xaml::RoutedEventArgs args) noexcept
   {
      change_active_page(selection_page);
   }

   HWND window;

   Xaml::Controls::Grid ui_root;

   Xaml::Controls::StackPanel selection_page;
   Collections::IVector<IInspectable> install_locations;
   Xaml::Controls::StackPanel searching_for_installs_status;

   Xaml::Controls::Grid installing_page;
   Xaml::Controls::TextBlock installing_status;
   Xaml::Controls::ProgressBar installing_progress_bar;

   Xaml::Controls::Grid permission_required_page;

   Xaml::Controls::Grid installation_failed_page;
   Collections::IVector<IInspectable> installation_failed_remaining_files;

   std::optional<game_disk_searcher> game_disk_searcher;
   std::optional<installer> installer;

   std::filesystem::path install_path;
};

}

auto make_app_mode_installer(Windows::UI::Xaml::Hosting::DesktopWindowXamlSource xaml_source)
   -> std::unique_ptr<app_ui_mode>
{
   return std::make_unique<app_mode_installer>(xaml_source);
}
