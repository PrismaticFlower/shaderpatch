#pragma once

#include "com_ptr.hpp"
#include "throw_if_failed.hpp"

#include <array>
#include <filesystem>
#include <initializer_list>
#include <optional>
#include <string>

#include <gsl/gsl>

#include <Windows.h>
#include <shobjidl.h>

namespace sp::win32 {

using Path = std::filesystem::path;

template<typename Dialog_class, const CLSID& clsid, const IID& iid>
std::optional<Path> file_dialog(std::initializer_list<COMDLG_FILTERSPEC> filters = {},
                                HWND owner = nullptr,
                                Path starting_dir = boost::filesystem::current_path(),
                                const std::wstring& filename = L""s)
{
   Com_ptr<Dialog_class> dialog;

   throw_if_failed(CoCreateInstance(clsid, nullptr, CLSCTX_ALL, iid,
                                    dialog.void_clear_and_assign()));

   throw_if_failed(dialog->SetFileTypes(filters.size(), filters.begin()));
   throw_if_failed(dialog->SetFileName(filename.c_str()));
   throw_if_failed(dialog->SetOptions(FOS_NOCHANGEDIR));

   Com_ptr<IShellItem> starting_item;

   throw_if_failed(
      SHCreateItemFromParsingName(starting_dir.c_str(), nullptr, IID_IShellItem,
                                  starting_item.void_clear_and_assign()));

   throw_if_failed(dialog->SetDefaultFolder(starting_item.get()));

   if (auto hr = dialog->Show(owner); hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
      return std::nullopt;
   }
   else {
      throw_if_failed(hr);
   }

   Com_ptr<IShellItem> item;

   throw_if_failed(dialog->GetResult(item.clear_and_assign()));

   wchar_t* name = nullptr;
   const auto name_free = gsl::finally([&name] {
      if (name) CoTaskMemFree(name);
   });

   item->GetDisplayName(SIGDN_FILESYSPATH, &name);

   return std::make_optional<Path>(name);
}

std::optional<Path> open_file_dialog(
   std::initializer_list<COMDLG_FILTERSPEC> filters = {}, HWND owner = nullptr,
   Path starting_dir = std::filesystem::current_path(),
   const std::wstring& filename = L""s)
{
   return file_dialog<IFileOpenDialog, CLSID_FileOpenDialog, IID_IFileOpenDialog>(
      filters, owner, std::move(starting_dir), filename);
}

std::optional<Path> save_file_dialog(
   std::initializer_list<COMDLG_FILTERSPEC> filters = {}, HWND owner = nullptr,
   Path starting_dir = std::filesystem::current_path(),
   const std::wstring& filename = L""s)
{
   return file_dialog<IFileSaveDialog, CLSID_FileSaveDialog, IID_IFileSaveDialog>(
      filters, owner, std::move(starting_dir), filename);
}
}
