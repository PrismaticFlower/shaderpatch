
#include "shader_patch_version.hpp"
#include "string_utilities.hpp"

#include <charconv>

namespace sp {

const std::string current_shader_patch_version_string =
   to_string(current_shader_patch_version);

using namespace std::literals;

auto to_string(const Shader_patch_version& version) noexcept -> std::string
{
   std::string str;

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

bool is_version_compatible(const Shader_patch_version& version,
                           const Shader_patch_version& reference) noexcept
{
   if (version.major != reference.major) return false;
   if (version.minor > reference.minor) return false;

   if (version.prerelease_stage == Shader_patch_prerelease_stage::preview ||
       reference.prerelease_stage == Shader_patch_prerelease_stage::preview) {

      if ((version.prerelease_stage == Shader_patch_prerelease_stage::preview &&
           reference.prerelease_stage == Shader_patch_prerelease_stage::preview) &&
          (version.prerelease_stage != reference.prerelease_stage))
         return false;

      if (version.prerelease == reference.prerelease &&
          version.minor == reference.minor)
         return false;
   }

   return true;
}

}
