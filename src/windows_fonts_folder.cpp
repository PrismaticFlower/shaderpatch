
#include "windows_fonts_folder.hpp"

#include <gsl/gsl>

#include <ShlObj.h>

namespace sp {

auto windows_fonts_folder() noexcept -> const std::filesystem::path&
{
   static std::filesystem::path path = []() -> std::filesystem::path {
      gsl::owner<wchar_t*> c_str_path = nullptr;
      auto c_str_path_free = gsl::finally([&] {
         if (c_str_path) CoTaskMemFree(c_str_path);
      });

      if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Fonts, 0x0, nullptr, &c_str_path))) {
         return c_str_path;
      }

      return L"C:\\Windows\\Fonts";
   }();

   return path;
}

}
