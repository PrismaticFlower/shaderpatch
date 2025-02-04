#include "game_memory.hpp"

#include "../logger.hpp"

#include <Windows.h>

namespace sp::game_support {

namespace {

const char signature_string[] = "Application";

struct Executable_info {
   const char* version = "";
   std::uintptr_t base_address = 0;
   std::uintptr_t signature_ptr = 0;
   std::uintptr_t leaf_patch_list_ptr = 0;
   bool is_debug_executable = false;
};

const Executable_info known_executables[] = {
   {
      .version = "GoG",

      .base_address = 0x00400000,

      .signature_ptr = 0x007a0698,
      .leaf_patch_list_ptr = 0x007ecaa8,
   },

   {
      .version = "Steam",

      .base_address = 0x00400000,

      .signature_ptr = 0x0079f834,
      .leaf_patch_list_ptr = 0x007ebad8,
   },

   {
      .version = "DVD",

      .base_address = 0x00400000,

      .signature_ptr = 0x007bf12c,
      .leaf_patch_list_ptr = 0x00801074,
   },

   {
      .version = "Modtools",

      .base_address = 0x00400000,

      .signature_ptr = 0x00a2b59c,
      .leaf_patch_list_ptr = 0x00accc84,

      .is_debug_executable = true,
   },
};

auto adjust_ptr(const std::uint32_t pointer, const std::uintptr_t base_address,
                const std::uintptr_t executable_base) -> void*
{
   return reinterpret_cast<void*>(pointer - base_address + executable_base);
}

int exception_filter(unsigned int code, [[maybe_unused]] EXCEPTION_POINTERS* ep)
{
   return code == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER
                                             : EXCEPTION_CONTINUE_SEARCH;
}

auto init_game_memory() noexcept -> Game_memory
{
   const std::uintptr_t executable_base =
      reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));

   for (const Executable_info& info : known_executables) {
      __try {
         if (std::memcmp(signature_string,
                         adjust_ptr(info.signature_ptr, info.base_address, executable_base),
                         sizeof(signature_string)) == 0) {
            log_fmt(Log_level::info, "Identified game version as: {}", info.version);

            return {
               .leaf_patch_list = adjust_ptr(info.leaf_patch_list_ptr,
                                             info.base_address, executable_base),

               .is_debug_executable = info.is_debug_executable,
            };
         }
      }
      __except (exception_filter(GetExceptionCode(), GetExceptionInformation())) {
         break;
      }
   }

   log(Log_level::warning,
       "Couldn't identify game. Some features that depend "
       "on reading/writing the game's memory will not work.");

   return {};
}

}

auto get_game_memory() noexcept -> const Game_memory&
{
   static Game_memory memory = init_game_memory();

   return memory;
}

}