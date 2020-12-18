#pragma once

#include "framework.hpp"

auto search_registry_locations_for_game_installs() noexcept
   -> std::vector<std::filesystem::path>;

class game_disk_searcher {
public:
   game_disk_searcher();

   ~game_disk_searcher();

   void get_discovered(std::function<void(std::filesystem::path)> callback) noexcept;

   bool completed() const noexcept;

private:
   std::mutex discovered_mutex;
   std::vector<std::filesystem::path> discovered_buffer;

   std::atomic_bool cancel_search = false;
   std::atomic_bool completed_search = false;

   std::future<void> search_future;
};
