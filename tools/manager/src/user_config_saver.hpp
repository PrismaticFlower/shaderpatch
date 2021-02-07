#pragma once
#include "framework.hpp"

#include "user_config.hpp"

class user_config_saver {
public:
   void enqueue_async_save(const user_config& config);

private:
   void save(const user_config& config) noexcept;

   std::shared_ptr<std::atomic_bool> _cancel_current_save;
   std::future<void> _future;
};

void save_user_config(const std::filesystem::path& path,
                      const user_config& config) noexcept;
