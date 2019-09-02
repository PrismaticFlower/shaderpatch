#pragma once

#include "file_dialogs.hpp"
#include "com_ptr.hpp"
#include "throw_if_failed.hpp"

#include <array>

#include <gsl/gsl>

namespace sp::win32 {

template<typename Dialog_class, const CLSID& clsid, const IID& iid>
std::optional<Path> file_dialog(std::initializer_list<COMDLG_FILTERSPEC> filters,
                                HWND owner, Path starting_dir,
                                const std::wstring& filename,
                                const FILEOPENDIALOGOPTIONS extra_options)
{
   Com_ptr<Dialog_class> dialog;

   throw_if_failed(CoCreateInstance(clsid, nullptr, CLSCTX_ALL, iid,
                                    dialog.void_clear_and_assign()));

   throw_if_failed(dialog->SetFileTypes(gsl::narrow_cast<UINT>(filters.size()),
                                        filters.begin()));
   throw_if_failed(dialog->SetFileName(filename.c_str()));
   throw_if_failed(dialog->SetOptions(FOS_NOCHANGEDIR | extra_options));

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

std::optional<Path> open_file_dialog(std::initializer_list<COMDLG_FILTERSPEC> filters,
                                     HWND owner, Path starting_dir,
                                     const std::wstring& filename)
{
   return file_dialog<IFileOpenDialog, CLSID_FileOpenDialog, IID_IFileOpenDialog>(
      filters, owner, std::move(starting_dir), filename, 0);
}

std::optional<Path> save_file_dialog(std::initializer_list<COMDLG_FILTERSPEC> filters,
                                     HWND owner, Path starting_dir,
                                     const std::wstring& filename)
{
   return file_dialog<IFileSaveDialog, CLSID_FileSaveDialog, IID_IFileSaveDialog>(
      filters, owner, std::move(starting_dir), filename, 0);
}

auto folder_dialog(HWND owner, Path starting_dir) -> std::optional<Path>
{
   Com_ptr<IFileOpenDialog> dialog;

   throw_if_failed(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL,
                                    IID_IFileOpenDialog,
                                    dialog.void_clear_and_assign()));

   throw_if_failed(dialog->SetOptions(FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST |
                                      FOS_NOCHANGEDIR | FOS_PICKFOLDERS));

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

}
