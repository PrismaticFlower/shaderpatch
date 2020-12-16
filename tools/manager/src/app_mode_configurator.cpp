#include "framework.hpp"

#include "app_ui_mode.hpp"
#include "config_ui.hpp"
#include "overloaded.hpp"
#include "user_config.hpp"
#include "user_config_descriptions.hpp"
#include "user_config_loader.hpp"
#include "user_config_saver.hpp"

using namespace std::literals;

namespace {

auto make_config_ui(std::shared_ptr<user_config> config) -> config::ui_root;

}

class app_mode_configurator final : public app_ui_mode {
public:
   void switch_to(winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource xaml_source) noexcept override
   {
      xaml_source.Content(config::create_xaml_ui_element(config_ui, config_changed));
   }

   void update([[maybe_unused]] MSG& msg) noexcept override
   {
      if (std::exchange(*config_changed, false)) {
         user_config_saver.enqueue_async_save(*config);
      }
   }

private:
   std::shared_ptr<user_config> config =
      std::make_shared<user_config>(load_user_config());
   config::ui_root config_ui = make_config_ui(config);
   std::shared_ptr<bool> config_changed = std::make_shared<bool>();
   user_config_saver user_config_saver;
};

auto make_app_mode_configurator() -> std::unique_ptr<app_ui_mode>
{
   return std::make_unique<app_mode_configurator>();
}

namespace {

auto make_config_ui(std::shared_ptr<user_config> config) -> config::ui_root
{
   config::ui_root root;

   const auto make_page = [&](std::wstring_view name,
                              user_config_value_vector& user_config_settings) {
      auto& page = root.pages.emplace_back();

      page.name = name;
      page.description = user_config_descriptions.at(name);

      auto& elements = page.elements;

      for (auto& setting : user_config_settings) {
         const auto init_common = [config](auto& control, auto& setting) {
            control.header = setting.name;
            control.description = user_config_descriptions.at(setting.name);
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
                       }},
            setting);
      }
   };

   make_page(L"Display"sv, config->display);
   make_page(L"Graphics"sv, config->graphics);
   make_page(L"Effects"sv, config->effects);
   make_page(L"Developer"sv, config->developer);

   root.shader_patch_enabled_value =
      std::shared_ptr<bool>{config, &config->enabled.value};

   return root;
}

}