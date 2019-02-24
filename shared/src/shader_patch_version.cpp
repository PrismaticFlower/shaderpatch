
#include "shader_patch_version.hpp"
#include "string_utilities.hpp"

#include <charconv>

namespace sp {

const std::string current_shader_patch_version_string =
   to_string(current_shader_patch_version);

using namespace std::literals;

auto to_string(const Shader_patch_version& version) noexcept -> std::string
{
   std::string str = "v"s;

   str += std::to_string(version.major);
   str += "."sv;
   str += std::to_string(version.minor);
   str += "."sv;
   str += std::to_string(version.patch);

   if (version.prerelease_stage == Shader_patch_prerelease_stage::none)
      return str;

   str += "-"sv;
   str += to_string(version.prerelease_stage);
   str += "."sv;
   str += std::to_string(version.prerelease);

   return str;
}

auto string_to_sp_version(const std::string_view string) noexcept -> Shader_patch_version
{
   const auto base_split = split_string_on(string, "v"sv);
   const auto major_split = split_string_on(base_split[1], "."sv);
   const auto minor_split = split_string_on(major_split[1], "."sv);
   const auto patch_split = split_string_on(minor_split[1], "-"sv);

   const auto prerelease_split = split_string_on(patch_split[1], "."sv);

   Shader_patch_version ver;

   std::from_chars(major_split[0].data(),
                   major_split[0].data() + major_split[0].size(), ver.major);
   std::from_chars(minor_split[0].data(),
                   minor_split[0].data() + minor_split[0].size(), ver.minor);
   std::from_chars(patch_split[0].data(),
                   patch_split[0].data() + patch_split[0].size(), ver.patch);

   if (!prerelease_split.empty()) {
      ver.prerelease_stage = string_to_sp_prerelease_stage(prerelease_split[0]);

      std::from_chars(prerelease_split[1].data(),
                      prerelease_split[1].data() + prerelease_split[1].size(),
                      ver.prerelease);
   }

   return ver;
}

bool is_version_compatible(const Shader_patch_version& version,
                           const Shader_patch_version& reference) noexcept
{
   if (version.major != reference.major) return false;
   if (version.minor > reference.minor) return false;

   if (version.prerelease_stage == Shader_patch_prerelease_stage::preview ||
       reference.prerelease_stage == Shader_patch_prerelease_stage::preview) {
      if (version.prerelease_stage != reference.prerelease_stage) return false;
      if (version.prerelease != reference.prerelease) return false;
   }

   return true;
}

}
