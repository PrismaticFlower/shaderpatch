
#include "bf2_log_monitor.hpp"
#include "file_helpers.hpp"
#include "string_utilities.hpp"

#include <cmath>
#include <filesystem>
#include <string_view>

#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"

#include <Windows.h>

using namespace std::literals;

namespace sp {

const auto log_name = L"BFront2.log";

struct BF2_log_entry {
   std::string_view severity;
   std::string_view file;
   std::string_view message;
};

namespace {

auto parse_log_file(const std::string_view log_file) noexcept
   -> std::vector<BF2_log_entry>
{
   std::vector<BF2_log_entry> results;
   results.reserve(2048);

   for (auto line_rest = split_string_on(log_file, "\r\n"sv); !line_rest[1].empty();
        line_rest = split_string_on(line_rest[1], "\r\n"sv)) {
      if (line_rest[0].starts_with("Message Severity"sv)) {
         const auto [file, message_rest] = split_string_on(line_rest[1], "\r\n"sv);
         const auto [message, remainder] = split_string_on(message_rest, "\r\n"sv);

         line_rest[1] = remainder;

         results.push_back({.severity = line_rest[0], .file = file, .message = message});
      }
      else if (line_rest[0].empty()) {
         results.push_back({.message = "\n"sv});
      }
      else {
         results.push_back({.message = line_rest[0]});
      }
   }

   return results;
}

auto get_severity_color(const std::string_view severity) -> ImVec4
{
   if (severity.ends_with("3"sv)) {
      return {1.0f, 0.1f, 0.1f, 1.0f};
   }
   else if (severity.ends_with("2"sv)) {
      return {1.0f, 1.0f, 0.25f, 1.0f};
   }

   return {1.0f, 1.0f, 1.0f, 1.0f};
}

bool test_filter(const BF2_log_entry& entry, const std::regex& filter) noexcept
{
   return std::regex_search(entry.severity.cbegin(), entry.severity.cend(), filter) ||
          std::regex_search(entry.file.cbegin(), entry.file.cend(), filter) ||
          std::regex_search(entry.message.cbegin(), entry.message.cend(), filter);
}

}

BF2_log_monitor::BF2_log_monitor()
{
   _thread = std::thread{[this] { run(); }};
}

BF2_log_monitor::~BF2_log_monitor()
{
   if (!SetEvent(_join_event.get())) {
      std::terminate();
   }

   _thread.join();
}

void BF2_log_monitor::show_imgui(const bool input_enabled) noexcept
{
   std::lock_guard lock{_mutex};

   ImGui::PushStyleVar(ImGuiStyleVar_Alpha, _transparency);

   ImGui::SetNextWindowSize({762, 844}, ImGuiCond_FirstUseEver);
   ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
   ImGui::Begin("BFront2.log");

   ImGui::PopStyleVar();

   if (input_enabled) {
      if (ImGui::InputText("Regex Filter", &_regex_str)) {
         if (!_regex_str.empty()) {
            try {
               _regex = std::regex{_regex_str, std::regex::icase};
            }
            catch (std::regex_error&) {
               _regex = std::nullopt;
            }
         }
         else {
            _regex = std::nullopt;
         }
      }

      ImGui::SameLine();
      ImGui::Checkbox("Auto Scroll", &_auto_scroll);

      ImGui::SliderFloat("Transparency", &_transparency, 0.05f, 1.0f);
      _transparency = std::clamp(_transparency, 0.05f, 1.0f);

      ImGui::SameLine();
      ImGui::Checkbox("Pin", &_overlay);
   }

   ImGui::BeginChild("Entries");

   for (const auto& entry : _log_entries) {
      if (_regex && !test_filter(entry, *_regex)) continue;

      if (!entry.severity.empty()) {
         ImGui::PushStyleColor(ImGuiCol_Text, get_severity_color(entry.severity));
         ImGui::TextUnformatted(&entry.severity.front(), &entry.severity.back() + 1);
         ImGui::PopStyleColor();
      }

      if (!entry.file.empty()) {
         ImGui::PushStyleColor(ImGuiCol_Text, {0.25f, 0.75f, 1.0f, 1.0f});
         ImGui::TextUnformatted(&entry.file.front(), &entry.file.back() + 1);
         ImGui::PopStyleColor();
      }

      if (!entry.message.empty()) {
         ImGui::TextUnformatted(&entry.message.front(), &entry.message.back() + 1);
      }
   }

   if (_auto_scroll) {
      ImGui::SetScrollY(ImGui::GetScrollMaxY());
   }

   ImGui::EndChild();

   if (ImGui::IsItemClicked()) _auto_scroll = false;

   ImGui::End();
}

bool BF2_log_monitor::overlay() const noexcept
{
   return _overlay;
}

void BF2_log_monitor::run() noexcept
{
   const auto file_notify =
      FindFirstChangeNotificationW(std::filesystem::current_path().c_str(),
                                   false, FILE_NOTIFY_CHANGE_LAST_WRITE);
   const auto file_notify_close =
      gsl::finally([file_notify] { FindCloseChangeNotification(file_notify); });

   if (file_notify == INVALID_HANDLE_VALUE) {
      std::terminate();
   }

   const auto log_path = std::filesystem::current_path() /= log_name;

   while (true) {
      const auto wait_objects = std::array{_join_event.get(), file_notify};
      const auto wait_status =
         WaitForMultipleObjects(wait_objects.size(), wait_objects.data(), false,
                                INFINITE);

      switch (wait_status) {
      case WAIT_OBJECT_0:
         return;
      case WAIT_OBJECT_0 + 1:
         if (std::filesystem::exists(log_path) &&
             std::filesystem::is_regular_file(log_path)) {
            std::lock_guard lock{_mutex};

            _file_contents = load_string_file(log_path);
            _log_entries = parse_log_file(_file_contents);
         }

         if (!FindNextChangeNotification(file_notify)) {
            std::terminate();
         }

         break;
      default:
         std::terminate();
      }
   }
}
}