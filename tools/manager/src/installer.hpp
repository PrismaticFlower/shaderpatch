#pragma once

#include "framework.hpp"

enum class installer_status {
   inprogress,
   completed,
   permission_denied,
   failure
};

class installer {
public:
   explicit installer(std::filesystem::path install_path);

   auto status() const noexcept -> installer_status;

   auto status_message() noexcept -> std::wstring;

   auto progress() const noexcept -> double;

   auto failed_to_remove_files() noexcept -> std::vector<std::filesystem::path>;

private:
   void install(std::filesystem::path install_path) noexcept;

   void failed_install_cleanup(const std::filesystem::path& install_path,
                               const std::vector<std::filesystem::path>& installed_files) noexcept;

   void set_message(std::wstring new_message) noexcept;

   std::atomic<installer_status> current_status = installer_status::inprogress;
   std::atomic<double> progress_percent = 0.0;

   std::mutex resource_mutex;

   std::wstring message;
   std::vector<std::filesystem::path> remaining_files;

   std::future<void> install_future;
};

[[noreturn]] void restart_and_install_as_admin(std::filesystem::path install_path) noexcept;
