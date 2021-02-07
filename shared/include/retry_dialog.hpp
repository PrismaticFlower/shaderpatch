#pragma once

#include <string>

#include <fmt/format.h>

#include <Windows.h>

namespace sp {

template<typename... Args>
inline bool retry_dialog(const std::string& title, const std::string_view text_fmt_str,
                         const Args&... args) noexcept
{
   const auto text = fmt::format(text_fmt_str, args...);

   return MessageBoxA(NULL, text.c_str(), title.c_str(), MB_RETRYCANCEL) == IDRETRY;
}

}
