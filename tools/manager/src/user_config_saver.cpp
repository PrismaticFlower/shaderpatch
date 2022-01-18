#include "framework.hpp"

#include "overloaded.hpp"
#include "string_helpers.hpp"
#include "user_config_descriptions.hpp"
#include "user_config_saver.hpp"

#include <algorithm>

using namespace std::literals;

constexpr auto temp_save_path = L"~TMP.shader patch.yml"sv;
constexpr auto save_path = L"shader patch.yml"sv;

void user_config_saver::enqueue_async_save(const user_config& config)
{
   if (_cancel_current_save) _cancel_current_save->store(true);
   _cancel_current_save = std::make_shared<std::atomic_bool>(false);

   if (_future.valid() && _future.wait_for(0s) != std::future_status::ready) {
      _future = std::async(std::launch::async, [this, config = config,
                                                old_future = std::move(_future),
                                                cancel = _cancel_current_save] {
         if (cancel->load()) return;

         old_future.wait();

         if (cancel->load()) return;

         save(config);
      });
   }
   else {
      _future = std::async(std::launch::async,
                           [this, config = config, cancel = _cancel_current_save] {
                              if (cancel->load()) return;

                              save(config);
                           });
   }
}

void user_config_saver::save(const user_config& config) noexcept
{
   save_user_config(temp_save_path, config);

   std::filesystem::rename(temp_save_path, save_path);
}

void save_user_config(const std::filesystem::path& path, const user_config& config) noexcept
{
   std::ofstream out{path};

   constexpr static auto to_utf8 = winrt::to_string;
   constexpr static auto printify =
      overloaded{[](const auto& v) { return v; },
                 [](const bool& v) { return v ? "yes"sv : "no"sv; },
                 [](const winrt::hstring& v) { return to_utf8(v); },
                 [](const std::array<std::uint32_t, 2>& v) {
                    return "["s + std::to_string(v[0]) + ", "s +
                           std::to_string(v[1]) + "]"s;
                 },
                 [](const std::array<std::uint8_t, 3>& v) {
                    return "["s + std::to_string(v[0]) + ", "s +
                           std::to_string(v[1]) + ", "s + std::to_string(v[2]) + "]"s;
                 }};
   constexpr static auto line_break = "\n\n"sv;
   constexpr static auto indention = "   "sv;
   constexpr static auto max_comment_line_size = 100;

   const auto write_value = [&](const auto& setting) {
      auto description = to_utf8(user_config_descriptions.at(setting.name));

      for (std::string_view line : lines_iterator{description}) {
         out << indention << "#"sv;

         std::size_t current_line_length = 0;

         for (auto& token : token_iterator{line}) {
            current_line_length += (token.size() + 1); // + 1 for ' '

            if (current_line_length >= max_comment_line_size) {
               out << '\n' << indention << "#"sv;
               current_line_length = 0;
            }

            out << ' ' << token;
         }

         out << '\n';
      }

      out << indention << to_utf8(setting.name) << ": "sv
          << printify(setting.value) << line_break;
   };

   out << to_utf8(config.enabled.name) << ": "sv
       << printify(config.enabled.value) << line_break;

   out << "Display: "sv << line_break;

   for (const auto& setting : config.display) std::visit(write_value, setting);

   out << "User Interface: "sv << line_break;

   for (const auto& setting : config.user_interface) {
      std::visit(write_value, setting);
   }

   out << "Graphics: "sv << line_break;

   for (const auto& setting : config.graphics) std::visit(write_value, setting);

   out << "Effects: "sv << line_break;

   for (const auto& setting : config.effects) std::visit(write_value, setting);

   out << "Developer: "sv << line_break;

   for (const auto& setting : config.developer) {
      std::visit(write_value, setting);
   }

   out.close();
}
