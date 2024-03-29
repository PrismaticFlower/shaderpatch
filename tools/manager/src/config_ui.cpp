#include "framework.hpp"

#include "config_ui.hpp"
#include "xaml_ui_helpers.hpp"

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::UI;

namespace config {

namespace {

constexpr double description_opacity = 0.85;
constexpr double description_top_padding = 8.0;
constexpr double description_bottom_padding = 16.0;
constexpr double description_max_width = 600.0;

constexpr double header_bottom_padding = 4.0;

constexpr double slider_width = 300.0;
constexpr double slider_value_padding = 8.0;

constexpr double uint_box_padding = 12.0;
constexpr double uint_box_width = (300.0 - uint_box_padding) / 2.0;

constexpr double combo_box_width = 300.0;

constexpr double text_box_width = 300.0;

constexpr double color_pick_button_length = 40.0;
constexpr double color_pick_button_margin = 10.0;

auto create_header_text_block(winrt::hstring string) -> Xaml::Controls::TextBlock
{
   Xaml::Controls::TextBlock text_block;
   text_block.Text(string);
   text_block.HorizontalAlignment(Xaml::HorizontalAlignment::Left);
   text_block.TextTrimming(Xaml::TextTrimming::CharacterEllipsis);
   text_block.Padding(Xaml::Thickness{0.0, 0.0, 0.0, header_bottom_padding});
   apply_text_style(text_block, text_style::subtitle);

   return text_block;
}

auto create_description_text_block(winrt::hstring string) -> Xaml::Controls::TextBlock
{
   Xaml::Controls::TextBlock text_block;
   text_block.Text(string);
   text_block.HorizontalAlignment(Xaml::HorizontalAlignment::Left);
   text_block.TextWrapping(Xaml::TextWrapping::WrapWholeWords);
   text_block.IsTextSelectionEnabled(true);
   text_block.Opacity(description_opacity);
   text_block.Padding(Xaml::Thickness{0.0, description_top_padding, 0.0,
                                      description_bottom_padding});
   text_block.MaxWidth(description_max_width);

   return text_block;
}

auto create_element(const toggle_switch& toggle_switch,
                    std::shared_ptr<bool> value_changed) -> Xaml::UIElement
{
   Xaml::Controls::ToggleSwitch toggle;
   toggle.HorizontalAlignment(Xaml::HorizontalAlignment::Left);
   toggle.IsOn(*toggle_switch.value);
   toggle.Toggled([value = toggle_switch.value,
                   value_changed](IInspectable element,
                                  [[maybe_unused]] Xaml::RoutedEventArgs args) {
      *value = element.as<Xaml::Controls::ToggleSwitch>().IsOn();
      *value_changed = true;
   });

   if (!toggle_switch.on_string.empty()) {
      toggle.OnContent(winrt::box_value(toggle_switch.on_string));
   }
   if (!toggle_switch.off_string.empty()) {
      toggle.OffContent(winrt::box_value(toggle_switch.off_string));
   }

   return toggle;
}

auto create_element(const uint_slider& uint_slider,
                    std::shared_ptr<bool> value_changed) -> Xaml::UIElement
{
   Xaml::Controls::StackPanel stack_panel;
   stack_panel.Orientation(Xaml::Controls::Orientation::Horizontal);

   Xaml::Controls::TextBlock value_text_block;
   value_text_block.Text(std::to_wstring(*uint_slider.value));
   value_text_block.HorizontalAlignment(Xaml::HorizontalAlignment::Left);
   value_text_block.VerticalAlignment(Xaml::VerticalAlignment::Center);
   value_text_block.Padding(Xaml::Thickness{slider_value_padding});

   Xaml::Controls::Slider slider;

   slider.Maximum(uint_slider.max);
   slider.Minimum(uint_slider.min);
   slider.Width(slider_width);
   slider.Value(*uint_slider.value);
   slider.HorizontalAlignment(Xaml::HorizontalAlignment::Left);
   slider.ValueChanged(
      [=, value = uint_slider.value](IInspectable sender,
                                     Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs e) {
         *value = static_cast<std::uint32_t>(e.NewValue());
         *value_changed = true;

         value_text_block.Text(std::to_wstring(*value));
      });

   stack_panel.Children().Append(slider);
   stack_panel.Children().Append(value_text_block);

   return stack_panel;
}

auto create_element(const uint2_boxes& uint2_boxes,
                    std::shared_ptr<bool> value_changed) -> Xaml::UIElement
{
   Xaml::Controls::StackPanel stack_panel;
   stack_panel.Orientation(Xaml::Controls::Orientation::Horizontal);

   for (std::size_t i = 0; i < uint2_boxes.subheaders.size(); ++i) {
      Xaml::Controls::TextBox value_box;
      value_box.Header(winrt::box_value(uint2_boxes.subheaders[i]));
      value_box.Text(std::to_wstring((*uint2_boxes.value)[i]));
      value_box.HorizontalAlignment(Xaml::HorizontalAlignment::Left);
      value_box.Width(uint_box_width);
      value_box.Margin(Xaml::Thickness{0, 0, uint_box_padding});
      value_box.BeforeTextChanging(
         [](Xaml::Controls::TextBox box,
            Xaml::Controls::TextBoxBeforeTextChangingEventArgs args) {
            auto new_text = args.NewText();

            args.Cancel(!std::all_of(new_text.cbegin(), new_text.cend(), std::iswdigit));
         });
      value_box.TextChanged([i, value = uint2_boxes.value,
                             value_changed](IInspectable box,
                                            Xaml::Controls::TextChangedEventArgs args) {
         try {
            (*value)[i] =
               std::stoul(std::wstring{box.as<Xaml::Controls::TextBox>().Text()});
            *value_changed = true;
         }
         catch (std::invalid_argument&) {
         }
         catch (std::out_of_range&) {
         }
      });

      stack_panel.Children().Append(value_box);
   }

   return stack_panel;
}

auto create_element(const combo_box& combo_box, std::shared_ptr<bool> value_changed)
   -> Xaml::UIElement
{
   Xaml::Controls::ComboBox box;
   box.HorizontalAlignment(Xaml::HorizontalAlignment::Left);
   box.Width(combo_box_width);

   for (const auto& item : combo_box.items) {
      box.Items().Append(winrt::box_value(item));
   }

   box.SelectedItem(winrt::box_value(*combo_box.value));
   box.SelectionChanged(
      [value = combo_box.value,
       value_changed](IInspectable box, Xaml::Controls::SelectionChangedEventArgs args) {
         *value = winrt::unbox_value<winrt::hstring>(
            box.as<Xaml::Controls::ComboBox>().SelectedItem());
         *value_changed = true;
      });

   return box;
}

auto create_element(const text_box& text_box, std::shared_ptr<bool> value_changed)
   -> Xaml::UIElement
{
   Xaml::Controls::TextBox box;
   box.Text(*text_box.value);
   box.HorizontalAlignment(Xaml::HorizontalAlignment::Left);
   box.Width(text_box_width);
   box.TextChanged([value = text_box.value,
                    value_changed](IInspectable box,
                                   Xaml::Controls::TextChangedEventArgs args) {
      *value = box.as<Xaml::Controls::TextBox>().Text();
      *value_changed = true;
   });

   return box;
}

auto create_element(const color_picker& color_picker,
                    std::shared_ptr<bool> value_changed) -> Xaml::UIElement
{
   Color color{255u, (*color_picker.value)[0], (*color_picker.value)[1],
               (*color_picker.value)[2]};

   Xaml::Controls::StackPanel stack_panel;
   stack_panel.Orientation(Xaml::Controls::Orientation::Horizontal);

   Xaml::Controls::TextBlock value_text_block;
   value_text_block.Text(std::to_wstring(color.R) + L" " + std::to_wstring(color.G) +
                         L" " + std::to_wstring(color.B));
   value_text_block.HorizontalAlignment(Xaml::HorizontalAlignment::Left);
   value_text_block.VerticalAlignment(Xaml::VerticalAlignment::Center);
   value_text_block.Padding(Xaml::Thickness{slider_value_padding});
   value_text_block.Width(100.0);

   Xaml::Media::SolidColorBrush button_brush;
   button_brush.Color(color);

   Xaml::Controls::Button button;

   button.Width(color_pick_button_length);
   button.Height(color_pick_button_length);
   button.Margin(Xaml::Thickness{color_pick_button_margin});
   button.Background(button_brush);
   button.HorizontalAlignment(Xaml::HorizontalAlignment::Left);

   Xaml::Controls::Flyout picker_flyout;

   Xaml::Controls::ColorPicker picker;

   picker.IsMoreButtonVisible(false);
   picker.IsColorSliderVisible(true);
   picker.IsColorChannelTextInputVisible(true);
   picker.IsHexInputVisible(true);
   picker.IsAlphaEnabled(false);
   picker.Color(color);
   picker.ColorChanged(
      [=, value = color_picker.value](IInspectable box,
                                      Xaml::Controls::ColorChangedEventArgs args) {
         Xaml::Media::SolidColorBrush button_brush;
         button_brush.Color(args.NewColor());
         button.Background(button_brush);

         value_text_block.Text(std::to_wstring(args.NewColor().R) + L" " +
                               std::to_wstring(args.NewColor().G) + L" " +
                               std::to_wstring(args.NewColor().B));

         *value = {args.NewColor().R, args.NewColor().G, args.NewColor().B};
         *value_changed = true;
      });

   picker_flyout.Content(picker);

   button.Flyout(picker_flyout);

   stack_panel.Children().Append(value_text_block);
   stack_panel.Children().Append(button);

   return stack_panel;
}

auto create_page(const ui_page& ui_page, std::shared_ptr<bool> value_changed)
   -> Xaml::UIElement
{
   Xaml::Controls::ScrollViewer scroll_viewer;
   Xaml::Controls::StackPanel stack_panel;
   stack_panel.Padding(Xaml::Thickness{16.0, 0.0});

   auto page_description_text_block =
      create_description_text_block(ui_page.description);

   page_description_text_block.Padding(
      Xaml::Thickness{0.0, 0.0, 0.0, description_bottom_padding});

   stack_panel.Children().Append(page_description_text_block);

   for (const auto& ui_element : ui_page.elements) {
      std::visit(
         [stack_panel, value_changed](const auto& ui_element) {
            auto element_header = create_header_text_block(ui_element.header);
            auto element_content = create_element(ui_element, value_changed);
            auto element_description_text_block =
               create_description_text_block(ui_element.description);

            stack_panel.Children().Append(element_header);
            stack_panel.Children().Append(element_content);
            stack_panel.Children().Append(element_description_text_block);
         },
         ui_element);
   }

   stack_panel.UpdateLayout();

   scroll_viewer.Content(stack_panel);
   scroll_viewer.Visibility(Xaml::Visibility::Collapsed);
   scroll_viewer.Name(ui_page.name);

   return scroll_viewer;
}

}

auto create_xaml_ui_element(const ui_root& root, std::shared_ptr<bool> value_changed)
   -> Xaml::Controls::NavigationView
{
   Xaml::Controls::NavigationView nav_view;
   nav_view.IsSettingsVisible(false);
   nav_view.IsPaneToggleButtonVisible(false);
   nav_view.IsBackButtonVisible(Xaml::Controls::NavigationViewBackButtonVisible::Collapsed);
   nav_view.IsBackEnabled(false);

   auto dep_obj = nav_view.as<Xaml::DependencyObject>();

   dep_obj.SetValue(nav_view.PaneDisplayModeProperty(),
                    winrt::box_value(Xaml::Controls::NavigationViewPaneDisplayMode::Left));
   dep_obj.SetValue(nav_view.DisplayModeProperty(),
                    winrt::box_value(Xaml::Controls::NavigationViewDisplayMode::Expanded));

   // Toggle Switch
   Xaml::Controls::ToggleSwitch toggle;
   toggle.VerticalAlignment(Xaml::VerticalAlignment::Bottom);
   toggle.HorizontalAlignment(Xaml::HorizontalAlignment::Left);
   toggle.IsOn(*root.shader_patch_enabled_value);
   toggle.OnContent(winrt::box_value(L"Shader Patch Enabled"));
   toggle.OffContent(winrt::box_value(L"Shader Patch Disabled"));
   toggle.Toggled([value = root.shader_patch_enabled_value,
                   value_changed](IInspectable element,
                                  [[maybe_unused]] Xaml::RoutedEventArgs args) {
      *value = element.as<Xaml::Controls::ToggleSwitch>().IsOn();
      *value_changed = true;
   });

   nav_view.PaneFooter(toggle);

   Xaml::Controls::Grid pages;

   for (const auto& page : root.pages) {
      nav_view.MenuItems().Append(winrt::box_value(page.name));
      pages.Children().Append(create_page(page, value_changed));
   }

   nav_view.Content(pages);
   nav_view.SelectionChanged(
      [pages](Xaml::Controls::NavigationView nav_view,
              [[maybe_unused]] Xaml::Controls::NavigationViewSelectionChangedEventArgs args) {
         auto selected = winrt::unbox_value<winrt::hstring>(args.SelectedItem());

         for (auto page : pages.Children()) {
            if (page.as<Xaml::Controls::Control>().Name() == selected) {
               nav_view.Header(args.SelectedItem());
               page.Visibility(Xaml::Visibility::Visible);
            }
            else {
               page.Visibility(Xaml::Visibility::Collapsed);
            }
         }
      });

   if (nav_view.MenuItems().Size() > 0) {
      nav_view.Header(nav_view.MenuItems().GetAt(0));
      nav_view.SelectedItem(nav_view.MenuItems().GetAt(0));
   }

   return nav_view;
}

}
