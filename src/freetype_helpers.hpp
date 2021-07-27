#pragma once

#include "logger.hpp"

#include <filesystem>

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
      log_and_terminate(
         "FreeType has reported an error. Unable to build font atlas!");
}

inline auto make_freetype_library() noexcept -> Freetype_ptr<FT_Library, FT_Done_FreeType>
{

   FT_Library raw_library = nullptr;

   freetype_checked_call(FT_Init_FreeType(&raw_library));

   return Freetype_ptr<FT_Library, FT_Done_FreeType>{raw_library};
}

inline auto make_freetype_face(FT_Library library,
                               const std::filesystem::path& font_path) noexcept
   -> Freetype_ptr<FT_Face, FT_Done_Face>
{
   FT_Face raw_face = nullptr;

   freetype_checked_call(FT_New_Face(library, font_path.string().c_str(), 0, &raw_face));

   return Freetype_ptr<FT_Face, FT_Done_Face>{raw_face};
}

}
