#pragma once

#include "framework.hpp"

auto load_user_config_descriptions()
   -> std::unordered_map<std::wstring_view, std::wstring_view>;

inline const std::unordered_map<std::wstring_view, std::wstring_view> user_config_descriptions =
   load_user_config_descriptions();
