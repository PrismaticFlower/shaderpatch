#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace sp {

enum class Shader_patch_prerelease_stage {
   none, // Not a prerelease.
   rc, // A release candidate, compatible with regular releases and other release candidates.
   preview // A preview release, incompatible with any other release.
};

struct Shader_patch_version {
   std::uint16_t major{};
   std::uint16_t minor{};
   std::uint16_t patch{};
   Shader_patch_prerelease_stage prerelease_stage{};
   std::uint16_t prerelease{};
};

inline const Shader_patch_version current_shader_patch_version{1, 4, 0,
                                                               Shader_patch_prerelease_stage::preview,
                                                               0};

extern const std::string current_shader_patch_version_string;

auto to_string(const Shader_patch_version& version) noexcept -> std::string;

auto string_to_sp_version(const std::string_view string) noexcept -> Shader_patch_version;

constexpr auto to_string(const Shader_patch_prerelease_stage stage) noexcept
   -> std::string_view
{
   using namespace std::literals;

   switch (stage) {
   case Shader_patch_prerelease_stage::rc:
      return "rc"sv;
   case Shader_patch_prerelease_stage::preview:
      return "preview"sv;
   }

   return ""sv;
}

constexpr auto string_to_sp_prerelease_stage(const std::string_view string) noexcept
   -> Shader_patch_prerelease_stage
{
   if (string == to_string(Shader_patch_prerelease_stage::rc))
      return Shader_patch_prerelease_stage::rc;

   if (string == to_string(Shader_patch_prerelease_stage::preview))
      return Shader_patch_prerelease_stage::preview;

   return Shader_patch_prerelease_stage::none;
}

bool is_version_compatible(const Shader_patch_version& version,
                           const Shader_patch_version& reference) noexcept;

}
