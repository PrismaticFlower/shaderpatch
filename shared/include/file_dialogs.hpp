#pragma once

#include <filesystem>
#include <initializer_list>
#include <optional>
#include <string>

#include <shobjidl.h>

namespace sp::win32 {

using Path = std::filesystem::path;

auto open_file_dialog(std::initializer_list<COMDLG_FILTERSPEC> filters = {},
                      HWND owner = nullptr,
                      Path starting_dir = std::filesystem::current_path(),
                      const std::wstring& filename = {}) -> std::optional<Path>;

auto save_file_dialog(std::initializer_list<COMDLG_FILTERSPEC> filters = {},
                      HWND owner = nullptr,
                      Path starting_dir = std::filesystem::current_path(),
                      const std::wstring& filename = {}) -> std::optional<Path>;

auto folder_dialog(HWND owner = nullptr,
                   Path starting_dir = std::filesystem::current_path())
   -> std::optional<Path>;

}
