#include "framework.hpp"

#include "user_config_loader.hpp"

#pragma warning(push)
#pragma warning(disable : 4996)

#include <yaml-cpp/yaml.h>

#pragma warning(pop)

using namespace std::literals;

auto load_user_config(const std::filesystem::path& path) -> user_config
{
   user_config config;

   try {
      const auto yaml = YAML::LoadFile(path.string());

      if (yaml["Shader Patch Enabled"s]) {
         config.enabled.value =
            yaml["Shader Patch Enabled"s].as<bool>(config.enabled.value);
      }

      const auto load_category = [](YAML::Node category,
                                    user_config_value_vector& settings) {
         if (!category) return;

         for (auto& setting : settings) {
            try {
               std::visit(
                  [&](auto& setting) {
                     auto utf8_name = winrt::to_string(setting.name);

                     if (!category[utf8_name]) return;

                     using setting_type = std::remove_reference_t<decltype(setting)>;

                     if constexpr (std::is_same_v<setting_type, percentage_user_config_value>) {
                        setting.value = std::max(category[utf8_name].as<std::uint32_t>(
                                                    setting.value),
                                                 setting.min_value);
                     }
                     else if constexpr (std::is_same_v<setting_type, uint2_user_config_value>) {
                        for (std::size_t i = 0; i < setting.value.size(); ++i) {
                           setting.value[i] = category[utf8_name][i].as<std::uint32_t>(
                              setting.value[i]);
                        }
                     }
                     else if constexpr (std::is_same_v<setting_type, enum_user_config_value>) {
                        auto value = category[utf8_name]
                                        ? winrt::to_hstring(
                                             category[utf8_name].as<std::string>())
                                        : setting.value;

                        if (std::any_of(setting.possible_values.cbegin(),
                                        setting.possible_values.cend(),
                                        [value](const auto& other) {
                                           return value == other;
                                        })) {
                           setting.value = value;
                        }
                     }
                     else if constexpr (std::is_same_v<setting_type, string_user_config_value>) {
                        setting.value =
                           category[utf8_name]
                              ? winrt::to_hstring(category[utf8_name].as<std::string>())
                              : setting.value;
                     }
                     else {
                        setting.value =
                           category[utf8_name].as<typename setting_type::value_type>(
                              setting.value);
                     }
                  },
                  setting);
            }
            catch (std::exception&) {
            }
         }
      };

      load_category(yaml["Display"s], config.display);
      load_category(yaml["Graphics"s], config.graphics);
      load_category(yaml["Effects"s], config.effects);
      load_category(yaml["Developer"s], config.developer);
   }
   catch (std::exception&) {
      // TODO: Report load failure to user?
   }

   return config;
}
