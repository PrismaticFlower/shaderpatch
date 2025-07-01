#pragma once

#include <string_view>
#include <unordered_map>

auto load_user_config_descriptions()
   -> std::unordered_map<std::string_view, std::string_view>;

inline const std::unordered_map<std::string_view, std::string_view> user_config_descriptions =
   load_user_config_descriptions();
