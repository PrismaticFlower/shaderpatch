
#include "logger.hpp"
#include "memory_mapped_file.hpp"
#include "smart_win32_handle.hpp"
#include "string_utilities.hpp"
#include "ucfb_editor.hpp"
#include "ucfb_writer.hpp"

#include <filesystem>
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

decltype(&CreateFileA) true_CreateFileA = CreateFileA;

extern "C" HANDLE WINAPI CreateFileA_hook(LPCSTR file_name,
                                          DWORD desired_access, DWORD share_mode,
                                          LPSECURITY_ATTRIBUTES security_attributes,
                                          DWORD creation_disposition,
                                          DWORD flags_and_attributes,
                                          HANDLE template_file)
{
   if (file_name && Ci_String_view{file_name} == R"(data\_lvl_pc\core.lvl)"_svci)
      return edit_core_lvl().release();

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
