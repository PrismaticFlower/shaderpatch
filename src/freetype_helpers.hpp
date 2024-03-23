#pragma once

#include "logger.hpp"

#include <filesystem>
#include <span>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace sp {

template<typename T, auto deleter>
struct Freetype_ptr {
   Freetype_ptr() = default;

   Freetype_ptr(T v) : ptr{v} {}

   Freetype_ptr(const Freetype_ptr&) = delete;
   auto operator=(const Freetype_ptr&) -> Freetype_ptr& = delete;

   Freetype_ptr(Freetype_ptr&& other) noexcept
   {
      std::swap(this->ptr, other.ptr);
   }

   auto operator=(Freetype_ptr&& other) noexcept -> Freetype_ptr&
   {
      std::swap(this->ptr, other.ptr);

      return *this;
   }

   ~Freetype_ptr()
   {
      if (ptr) deleter(ptr);
   }

   auto get() const noexcept -> T
   {
      return ptr;
   }

   operator T() const noexcept
   {
      return get();
   }

private:
   T ptr = nullptr;
};

inline void freetype_checked_call(const FT_Error error)
{
   if (error)
      log_and_terminate_fmt("FreeType has reported an error: {}"sv,
                            FT_Error_String(error));
}

auto make_freetype_library() noexcept -> Freetype_ptr<FT_Library, FT_Done_FreeType>;

auto make_freetype_face(FT_Library library, std::span<const FT_Byte> data) noexcept
   -> Freetype_ptr<FT_Face, FT_Done_Face>;

auto load_font_data(const std::filesystem::path& font_path) -> std::vector<FT_Byte>;

}
