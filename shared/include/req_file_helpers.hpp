#pragma once

#include "small_function.hpp"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace sp {

auto parse_req_file(const std::filesystem::path& filepath,
                    std::string_view platform = "pc")
   -> std::vector<std::pair<std::string, std::vector<std::string>>>;

void emit_req_file(const std::filesystem::path& filepath,
                   const std::vector<std::pair<std::string, std::vector<std::string>>>& key_sections);

void parse_files_req_file(const std::filesystem::path& filepath,
                          Small_function<void(std::string entry) noexcept> callback);

template<typename SequenceContainer>
inline void parse_files_req_file(const std::filesystem::path& filepath,
                                 SequenceContainer& container)
{
   parse_files_req_file(
      filepath, [&](std::string entry) noexcept {
         container.emplace_back(std::move(entry));
      });
}

}
