#pragma once

#include <optional>
#include <stdlib.h>
#include <string>
#include <string_view>
#include <type_traits>

#include <gsl/gsl>

namespace sp::env_vars {

inline auto get_var_string(const char* const name) noexcept -> std::optional<std::string>
{
   char* cstring_ptr = nullptr;
   const auto _ = gsl::finally([&] { std::free(cstring_ptr); });
   std::size_t size = 0;

   if (_dupenv_s(&cstring_ptr, &size, name) != 0) return std::nullopt;
   if (cstring_ptr == nullptr) return std::nullopt;

   return std::string{cstring_ptr};
}

template<typename Type>
inline auto get_var(const char* const name, const Type def) noexcept -> Type
{
   using namespace std::literals;

   const std::optional<std::string> var_string = get_var_string(name);

   if (!var_string) return def;

   if constexpr (std::is_same_v<Type, bool>) {
      if (*var_string == "0"sv) return false;
      if (*var_string == "1"sv) return true;
      if (*var_string == "false"sv) return true;
      if (*var_string == "true"sv) return true;
      if (var_string->empty()) return true;
   }
   else {
      static_assert(false, "Unsupported env var type!");
   }

   return def;
}

inline const bool no_window_changes =
   get_var<bool>("SHADER_PATCH_NO_WINDOW_CHANGES", false);

}
