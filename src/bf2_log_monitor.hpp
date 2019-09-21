#pragma once

#include "smart_win32_handle.hpp"

#include <mutex>
#include <optional>
#include <regex>
#include <string>
#include <thread>
#include <vector>

namespace sp {

struct BF2_log_entry;

class BF2_log_monitor {
public:
   BF2_log_monitor();

   ~BF2_log_monitor();

   BF2_log_monitor(const BF2_log_monitor&) = delete;
   BF2_log_monitor& operator=(const BF2_log_monitor&) = delete;
   BF2_log_monitor(BF2_log_monitor&&) = delete;
   BF2_log_monitor& operator=(BF2_log_monitor&&) = delete;

   void show_imgui(const bool input_enabled) noexcept;

   bool overlay() const noexcept;

private:
   void run() noexcept;

   std::thread _thread;
   win32::Unique_handle _join_event{CreateEventW(nullptr, true, false, nullptr)};
   mutable std::mutex _mutex;
   std::string _file_contents;
   std::vector<BF2_log_entry> _log_entries;

   std::string _regex_str = "";
   std::optional<std::regex> _regex = std::nullopt;
   bool _auto_scroll = true;
   bool _overlay = false;
   float _transparency = 1.0f;
};

}