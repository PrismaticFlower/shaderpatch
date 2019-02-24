
#include "logger.hpp"
#include "memory_mapped_file.hpp"
#include "shader_patch_version.hpp"
#include "smart_win32_handle.hpp"
#include "string_utilities.hpp"
#include "ucfb_editor.hpp"
#include "ucfb_writer.hpp"

#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string_view>

#include <Windows.h>
#include <detours.h>
#include <io.h>

namespace sp {

using namespace std::literals;

namespace {

auto create_tmp_file() -> std::pair<std::ofstream, win32::Unique_handle>
{
   const auto temp_directory = std::filesystem::temp_directory_path();

   std::array<wchar_t, MAX_PATH> tmp_file_path{};

   GetTempFileNameW(temp_directory.c_str(), nullptr, 0, tmp_file_path.data());

   if (const auto code = GetLastError(); code != ERROR_SUCCESS) {
      log_and_terminate("Unable to get temporary file name. Error code is "sv,
                        std::hex, code);
   }

   win32::Unique_handle file{
      CreateFileW(tmp_file_path.data(), GENERIC_READ | GENERIC_WRITE,
                  FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
                  FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr)};

   if (file.get() == INVALID_HANDLE_VALUE) {
      log_and_terminate("Unable to create temporary file. Error code is "sv,
                        std::hex, GetLastError());
   }

   std::ofstream stream = [&] {
      const auto crt_desc = _open_osfhandle(reinterpret_cast<std::intptr_t>(
                                               duplicate_handle(file).release()),
                                            0);

      if (crt_desc == -1)
         log_and_terminate("Unable to create temporary file. Unknown error!"sv);

      FILE* const crt_file = _wfdopen(crt_desc, L"r+b");

      if (!crt_file)
         log_and_terminate("Unable to create temporary file. errno was "sv, errno);

      return std::ofstream{crt_file};
   }();

   return {std::move(stream), std::move(file)};
}

auto edit_core_lvl() noexcept -> win32::Unique_handle
{
   constexpr static auto is_parent = [](const auto) noexcept
   {
      return false;
   };

   win32::Memeory_mapped_file file{"data/_lvl_pc/core.lvl"sv};

   auto core_editor = [] {
      win32::Memeory_mapped_file file{"data/_lvl_pc/core.lvl"sv};

      return ucfb::Editor{ucfb::Reader_strict<"ucfb"_mn>{file.bytes()}, is_parent};
   }();

   // Strip out stock shader chunks.
   for (auto it = ucfb::find(core_editor, "SHDR"_mn); it != core_editor.end();
        it = ucfb::find(it, core_editor.end(), "SHDR"_mn)) {
      it = core_editor.erase(it);
   }

   const auto shader_decls_editor = [] {
      win32::Memeory_mapped_file file{"data/shaderpatch/shader_declarations.lvl"sv};

      return ucfb::Editor{ucfb::Reader_strict<"ucfb"_mn>{file.bytes()}, is_parent};
   }();

   core_editor.insert(core_editor.end(), shader_decls_editor.cbegin(),
                      shader_decls_editor.cend());

   auto [ostream, file_handle] = create_tmp_file();

   // Output new core.lvl to temp file
   {
      ucfb::Writer writer{ostream};

      core_editor.assemble(writer);
   }

   ostream.close();

   SetFilePointer(file_handle.get(), 0, nullptr, FILE_BEGIN);

   SetLastError(ERROR_SUCCESS);

   return std::move(file_handle);
}

auto edit_ingame_lvl() noexcept -> win32::Unique_handle
{
   constexpr static auto is_parent = [](const Magic_number mn) noexcept
   {
      if (mn == "scr_"_mn) return true;

      return false;
   };

   win32::Memeory_mapped_file file{"data/_lvl_pc/ingame.lvl"sv};

   auto common_editor = [] {
      win32::Memeory_mapped_file file{"data/_lvl_pc/ingame.lvl"sv};

      return ucfb::Editor{ucfb::Reader_strict<"ucfb"_mn>{file.bytes()}, is_parent};
   }();

   // Rename "interface_util" script.
   for (auto& chunk : common_editor) {
      if (chunk.first != "scr_"_mn) continue;

      // Check if this script is interface_util.
      if (![&] {
             // The first chunk is a `scr_` chunk is always `NAME`.
             const auto name =
                ucfb::make_strict_reader<"NAME"_mn>(
                   std::get<ucfb::Editor_parent_chunk>(chunk.second).begin())
                   .read_string();

             return view_as_ci_string(name) == "game_interface"_svci;
          }())
         continue;

      // If it is, rename it.
      auto& name_editor = std::get<ucfb::Editor_data_chunk>(
         std::get<ucfb::Editor_parent_chunk>(chunk.second)[0].second);

      name_editor.clear();
      name_editor.writer().write(
         "_SP_{26DE35C1-38C2-4BCD-B0B5-6C811C1C20CA}_game_interface"sv);

      break;
   }

   // Append new interface_util script.
   {
      auto& scr_chunk = std::get<ucfb::Editor_parent_chunk>(
         common_editor.emplace_back("scr_"_mn, ucfb::Editor_parent_chunk{}).second);

      scr_chunk.reserve(3);

      scr_chunk.emplace_back("NAME"_mn, [] {
         ucfb::Editor_data_chunk name;

         name.writer().write("game_interface"sv);

         return name;
      }());

      scr_chunk.emplace_back("INFO"_mn, [] {
         ucfb::Editor_data_chunk info;

         info.writer().write_unaligned(std::int8_t{0});

         return info;
      }());

      scr_chunk.emplace_back("BODY"_mn, [] {
         ucfb::Editor_data_chunk body;

         std::ostringstream script;

         script << R"(print "Loading Shader Patch Scripting Extenions...")"sv << '\n';
         script << "gSP_Installed = true"sv << '\n';
         script << "local spVersion = { major = "sv
                << current_shader_patch_version.major << ", minor = "sv
                << current_shader_patch_version.minor << ", patch = "sv
                << current_shader_patch_version.patch << ", prerelease_stage = "sv
                << std::quoted(to_string(current_shader_patch_version.prerelease_stage))
                << ", prerelease = "sv
                << current_shader_patch_version.prerelease << " }"sv << '\n';
         script << "local spVersionString = "sv
                << std::quoted(current_shader_patch_version_string) << '\n';

         script << R"(
function SP_TestVersion(major, minor, patch, prerelease_stage, prerelease)
   if (major ~= spVersion.major) then return false end
   if (minor > spVersion.minor) then return false end

   if (prerelease_stage == "preview" or spVersion.prerelease_stage == "preview") then
      if (prerelease_stage ~= spVersion.prerelease_stage) then return false end
      if (prerelease ~= spVersion.prerelease) then return false end
   end

   return true
end

ScriptCB_DoFile "_SP_{26DE35C1-38C2-4BCD-B0B5-6C811C1C20CA}_game_interface"

print(string.format("Shader Patch Scripting Extenions loaded. Shader Patch version is %s", spVersionString))

)"sv;

         // Write out the string as a span of unaligned chars to avoid the null
         // terminator as this will cause the Lua loader to fail.
         body.writer().write_unaligned(gsl::make_span(script.str()));

         // Manually pad out the chunk with new lines, once again to avoid nulls.
         if (const auto remainder = body.size() % 4u; remainder != 0) {
            constexpr auto newline = std::byte{'\n'};
            constexpr std::array newlines{newline, newline, newline, newline};

            body.insert(body.cend(), newlines.begin(),
                        newlines.begin() + (4 - remainder));
         }

         return body;
      }());
   }

   auto [ostream, file_handle] = create_tmp_file();

   // Output new common.lvl to temp file
   {
      ucfb::Writer writer{ostream};

      common_editor.assemble(writer);
   }

   ostream.close();

   SetFilePointer(file_handle.get(), 0, nullptr, FILE_BEGIN);

   SetLastError(ERROR_SUCCESS);

   return std::move(file_handle);
}

auto get_sp_api_script() noexcept -> win32::Unique_handle
{
   constexpr static auto is_parent = [](const Magic_number mn) noexcept
   {
      if (mn == "scr_"_mn) return true;

      return false;
   };

   ucfb::Editor editor;

   // Append new interface_util script.
   {
      auto& scr_chunk = std::get<ucfb::Editor_parent_chunk>(
         editor.emplace_back("scr_"_mn, ucfb::Editor_parent_chunk{}).second);

      scr_chunk.reserve(3);

      scr_chunk.emplace_back("NAME"_mn, [] {
         ucfb::Editor_data_chunk name;

         name.writer().write("shader_patch_api"sv);

         return name;
      }());

      scr_chunk.emplace_back("INFO"_mn, [] {
         ucfb::Editor_data_chunk info;

         info.writer().write_unaligned(std::int8_t{0});

         return info;
      }());

      scr_chunk.emplace_back("BODY"_mn, [] {
         ucfb::Editor_data_chunk body;

         std::ostringstream script;

         script << R"(print "Loading Shader Patch Scripting API...")"sv << '\n';
         script << "gSP_Installed = true"sv << '\n';
         script << "local spVersion = { major = "sv
                << current_shader_patch_version.major << ", minor = "sv
                << current_shader_patch_version.minor << ", patch = "sv
                << current_shader_patch_version.patch << ", prerelease_stage = "sv
                << std::quoted(to_string(current_shader_patch_version.prerelease_stage))
                << ", prerelease = "sv
                << current_shader_patch_version.prerelease << " }"sv << '\n';
         script << "local spVersionString = "sv
                << std::quoted(current_shader_patch_version_string) << '\n';

         script << R"(
function SP_TestVersion(major, minor, patch, prerelease_stage, prerelease)
   if (major ~= spVersion.major) then return false end
   if (minor > spVersion.minor) then return false end

   if (prerelease_stage == "preview" or spVersion.prerelease_stage == "preview") then
      if (prerelease_stage ~= spVersion.prerelease_stage) then return false end
      if (prerelease ~= spVersion.prerelease) then return false end
   end

   return true
end

print(string.format("Shader Patch Scripting API loaded. Shader Patch version is %s", spVersionString))

)"sv;

         // Write out the string as a span of unaligned chars to avoid the null
         // terminator as this will cause the Lua loader to fail.
         body.writer().write_unaligned(gsl::make_span(script.str()));

         // Manually pad out the chunk with new lines, once again to avoid nulls.
         if (const auto remainder = body.size() % 4u; remainder != 0) {
            constexpr auto newline = std::byte{'\n'};
            constexpr std::array newlines{newline, newline, newline, newline};

            body.insert(body.cend(), newlines.begin(),
                        newlines.begin() + (4 - remainder));
         }

         return body;
      }());
   }

   auto [ostream, file_handle] = create_tmp_file();

   // Output script to temp file.
   {
      ucfb::Writer writer{ostream};

      editor.assemble(writer);
   }

   ostream.close();

   SetFilePointer(file_handle.get(), 0, nullptr, FILE_BEGIN);

   SetLastError(ERROR_SUCCESS);

   return std::move(file_handle);
}

decltype(&CreateFileA) true_CreateFileA = CreateFileA;

extern "C" HANDLE WINAPI CreateFileA_hook(LPCSTR file_name,
                                          DWORD desired_access, DWORD share_mode,
                                          LPSECURITY_ATTRIBUTES security_attributes,
                                          DWORD creation_disposition,
                                          DWORD flags_and_attributes,
                                          HANDLE template_file)
{
   if (file_name) {
      if (Ci_String_view{file_name} == R"(data\_lvl_pc\core.lvl)"_svci)
         return edit_core_lvl().release();
      else if (Ci_String_view{file_name} == R"(data\_lvl_pc\shader_patch_api.script)"_svci)
         return get_sp_api_script().release();
   }

   return true_CreateFileA(file_name, desired_access, share_mode,
                           security_attributes, creation_disposition,
                           flags_and_attributes, template_file);
}

}

void install_file_hooks() noexcept
{
   bool failure = true;

   if (DetourTransactionBegin() == NO_ERROR) {
      if (DetourAttach(&reinterpret_cast<PVOID&>(true_CreateFileA),
                       CreateFileA_hook) == NO_ERROR) {
         if (DetourTransactionCommit() == NO_ERROR) {
            failure = false;
         }
      }
   }

   if (failure) {
      log_and_terminate("Failed to install file hooks."sv);
   }
}
}
