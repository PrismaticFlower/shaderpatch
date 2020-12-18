#pragma once

#include "framework.hpp"

#include <shobjidl.h>

inline auto open_file_dialog(COMDLG_FILTERSPEC filter)
   -> std::optional<std::filesystem::path>
{
   winrt::com_ptr<IFileOpenDialog> dialog;

   if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL,
                               IID_IFileOpenDialog, dialog.put_void()))) {
      return std::nullopt;
   }

   if (FAILED(dialog->SetFileTypes(1, &filter))) {
      return std::nullopt;
   }

   if (FAILED(dialog->SetOptions(FOS_NOCHANGEDIR | FOS_FILEMUSTEXIST |
                                 FOS_DONTADDTORECENT))) {
      return std::nullopt;
   }

   if (FAILED(dialog->Show(nullptr))) {
      return std::nullopt;
   }

   winrt::com_ptr<IShellItem> item;

   if (FAILED(dialog->GetResult(item.put()))) {
      return std::nullopt;
   }

   wchar_t* name_raw_owning_ptr = nullptr;

   if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &name_raw_owning_ptr))) {
      return std::nullopt;
   }

   const std::uint32_t str_len = std::wstring_view{name_raw_owning_ptr}.size();

   winrt::com_array<wchar_t> name{std::exchange(name_raw_owning_ptr, nullptr),
                                  str_len, winrt::take_ownership_from_abi};

   return std::make_optional<std::filesystem::path>(
      std::wstring_view{name.data(), name.size()});
}
