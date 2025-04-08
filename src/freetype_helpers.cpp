#include "freetype_helpers.hpp"

#include <Windows.h>

#include <gsl/gsl>

namespace sp {

auto make_freetype_library() noexcept -> Freetype_ptr<FT_Library, FT_Done_FreeType>
{

   FT_Library raw_library = nullptr;

   freetype_checked_call(FT_Init_FreeType(&raw_library));

   return Freetype_ptr<FT_Library, FT_Done_FreeType>{raw_library};
}

auto load_font_data(const std::filesystem::path& font_path) -> std::vector<FT_Byte>
{
   HANDLE file =
      CreateFileW(font_path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                  OPEN_EXISTING,
                  FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL, nullptr);
   auto close_file = gsl::finally([&] {
      if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
   });

   file = CreateFileW(font_path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                      OPEN_EXISTING,
                      FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL, nullptr);

   if (file == INVALID_HANDLE_VALUE) {
      const DWORD system_error = GetLastError();

      log_and_terminate_fmt(
         "Failed to open  font file '{}'. Reason: {}", font_path.string(),
         std::system_category().default_error_condition(system_error).message());
   }

   LARGE_INTEGER file_size{.QuadPart = std::numeric_limits<LONGLONG>::max()};

   if (!GetFileSizeEx(file, &file_size)) {
      log_and_terminate_fmt("Failed to get fallback font file '{}' size.",
                            font_path.string());
   }

   if (file_size.QuadPart > std::numeric_limits<DWORD>::max()) {
      log_and_terminate_fmt("Fallback font file '{}' is too big to read. Max "
                            "readable size is 4294967295, file is {}.",
                            font_path.string(), file_size.QuadPart);
   }

   std::vector<FT_Byte> bytes;
   bytes.resize(static_cast<std::size_t>(file_size.QuadPart));

   DWORD read_bytes = 0;

   if (!ReadFile(file, bytes.data(), static_cast<DWORD>(bytes.size()),
                 &read_bytes, nullptr)) {
      const DWORD system_error = GetLastError();

      log_and_terminate_fmt(
         "Erroring reading fallback font file '{}'. Reason: {} Bytes Read: "
         "{}/{}",
         font_path.string(),
         std::system_category().default_error_condition(system_error).message(),
         read_bytes, file_size.QuadPart);
   }

   return bytes;
}

auto make_freetype_face(FT_Library library, std::span<const FT_Byte> data) noexcept
   -> Freetype_ptr<FT_Face, FT_Done_Face>
{
   FT_Face raw_face = nullptr;

   if (FT_Error error =
          FT_New_Memory_Face(library, data.data(), data.size(), 0, &raw_face);
       error) {
      log_and_terminate_fmt("FreeType has reported an error: {}"sv,
                            FT_Error_String(error));
   }

   return Freetype_ptr<FT_Face, FT_Done_Face>{raw_face};
}
}