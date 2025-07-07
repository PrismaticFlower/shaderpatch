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

   std::uintptr_t light_list_ptr = 0;
   std::uintptr_t global_dir_lights_ptr = 0;

   std::uintptr_t view_near_plane = 0;

   std::uintptr_t near_scene_fade_start_ptr = 0;
   std::uintptr_t near_scene_fade_end_ptr = 0;
   std::uintptr_t far_scene_range_ptr = 0;
   std::uintptr_t far_scene_enabled_ptr = 0;

   bool is_debug_executable = false;
};

const Executable_info known_executables[] = {
   {
      .version = "GoG",

      .base_address = 0x00400000,

      .signature_ptr = 0x007a0698,

      .leaf_patch_list_ptr = 0x007ecaa8,

      .light_list_ptr = 0x007e0014,
      .global_dir_lights_ptr = 0x009623d4,

      .view_near_plane = 0x008f826c,

      .near_scene_fade_start_ptr = 0x008f8270,
      .near_scene_fade_end_ptr = 0x008f8278,
      .far_scene_range_ptr = 0x008f8280,

      .far_scene_enabled_ptr = 0x008f828c,
   },

   {
      .version = "Steam",

      .base_address = 0x00400000,

      .signature_ptr = 0x0079f834,

      .leaf_patch_list_ptr = 0x007ebad8,

      .view_near_plane = 0x008f6dcc,

      .near_scene_fade_start_ptr = 0x008f6dd0,
      .near_scene_fade_end_ptr = 0x008f6dd8,
      .far_scene_range_ptr = 0x008f6de0,

      .far_scene_enabled_ptr = 0x008f6dec,
   },

   {
      .version = "DVD",

      .base_address = 0x00400000,

      .signature_ptr = 0x007bf12c,
      .leaf_patch_list_ptr = 0x00801074,

      .view_near_plane = 0x01d72108,

      .near_scene_fade_start_ptr = 0x01d72218,
      .near_scene_fade_end_ptr = 0x01d7210c,
      .far_scene_range_ptr = 0x01d7212c,

      .far_scene_enabled_ptr = 0x01d720e8,
   },

   {
      .version = "Modtools",

      .base_address = 0x00400000,

      .signature_ptr = 0x00a2b59c,
      .leaf_patch_list_ptr = 0x00accc84,

      .view_near_plane = 0x00e5ba44,

      .near_scene_fade_start_ptr = 0x00e5bb50,
      .near_scene_fade_end_ptr = 0x00e5ba48,

      .far_scene_range_ptr = 0x00e5ba68,

      .far_scene_enabled_ptr = 0x00e5ba24,

      .is_debug_executable = true,
   },
};

template<typename T = void>
auto adjust_ptr(const std::uint32_t pointer, const std::uintptr_t base_address,
                const std::uintptr_t executable_base) -> T*
{
   if (pointer == 0) return nullptr;

   return reinterpret_cast<T*>(pointer - base_address + executable_base);
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
               .leaf_patch_list =
                  adjust_ptr<structures::LeafPatchListNode>(info.leaf_patch_list_ptr,
                                                            info.base_address,
                                                            executable_base),

               .light_list = adjust_ptr<structures::RedLightList>(info.light_list_ptr,
                                                                  info.base_address,
                                                                  executable_base),
               .global_dir_lights =
                  adjust_ptr<structures::RedDirectionalLight*>(info.global_dir_lights_ptr,
                                                               info.base_address,
                                                               executable_base),

               .view_near_plane = adjust_ptr<float>(info.view_near_plane,
                                                    info.base_address, executable_base),

               .near_scene_fade_start =
                  adjust_ptr<float>(info.near_scene_fade_start_ptr,
                                    info.base_address, executable_base),
               .near_scene_fade_end =
                  adjust_ptr<float>(info.near_scene_fade_end_ptr,
                                    info.base_address, executable_base),
               .far_scene_range = adjust_ptr<float>(info.far_scene_range_ptr,
                                                    info.base_address, executable_base),

               .far_scene_enabled = adjust_ptr<bool>(info.far_scene_enabled_ptr,
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