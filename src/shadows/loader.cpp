#include "loader.hpp"
#include "input.hpp"
#include "shadow_world.hpp"

#include "read/read_entity_class.hpp"
#include "read/read_game_model.hpp"
#include "read/read_model.hpp"
#include "read/read_texture.hpp"
#include "read/read_world.hpp"

#include "../logger.hpp"

#include <atomic>
#include <shared_mutex>
#include <string>
#include <thread>
#include <vector>

#include "ucfb_file_reader.hpp"

namespace sp::shadows {

namespace {

void load(const std::string& file_name) noexcept
{
   try {
      log_fmt(Log_level::info, "Loading '{}' for Shader Patch...", file_name);

      ucfb::File_reader file{file_name};

      while (file) {
         auto child = file.read_child();

         if (child.magic_number() == "tex_"_mn) {
            // Special case, has multiple children so it will add to shadow_world directly.

            read_texture(child);
         }
         else if (child.magic_number() == "skel"_mn) {
            // TODO: Handle Skeletons (will eventually be needed for attached leaf patches)
         }
         else if (child.magic_number() == "modl"_mn) {
            shadow_world.add_model(read_model(child));
         }
         else if (child.magic_number() == "gmod"_mn) {
            shadow_world.add_game_model(read_game_model(child));
         }
         else if (child.magic_number() == "entc"_mn) {
            shadow_world.add_entity_class(read_entity_class(child));
         }
         else if (child.magic_number() == "wrld"_mn) {
            // Special case, has multiple children so it will add to shadow_world directly.
            read_world(child, false);
         }
         else if (child.magic_number() == "lvl_"_mn) {
         }
      }
   }
   catch (std::exception& e) {
      log_fmt(Log_level::warning, "Failed to load '{}' for Shader Patch. Brief and unhelpful reason: {}",
              file_name, e.what());
   }
}

struct Loader {
   Loader() noexcept
   {
      _work_thread = std::thread{[this] { loader_loop(); }};
   }

   ~Loader()
   {
      _work_queue_size.store(-1);
      _work_queue_size.notify_one();

      _work_thread.join();
   }

   void queue_load_lvl(const char* file_name) noexcept
   {
      {
         std::scoped_lock lock{_work_queue_mutex};

         _work_queue.push_back({.file_name = file_name});
      }

      _work_queue_size += 1;
      _work_queue_size.notify_one();
   }

   void wait_all_loaded() noexcept
   {
      while (true) {
         const std::int32_t queue_size = _work_queue_size.load();

         if (queue_size < 1) return;

         _work_queue_size.wait(queue_size);
      }
   }

private:
   void loader_loop() noexcept
   {
      while (true) {
         _work_queue_size.wait(0);

         const std::int32_t queue_size = _work_queue_size.load();

         if (queue_size < 0) return;

         Work_item item;

         {
            std::scoped_lock lock{_work_queue_mutex};

            item = std::move(_work_queue.front());

            _work_queue.erase(_work_queue.begin());
         }

         load(item.file_name);

         _work_queue_size.fetch_sub(1);
         _work_queue_size.notify_one();
      }
   }

   struct Work_item {
      std::string file_name;
   };

   std::thread _work_thread;

   std::shared_mutex _work_queue_mutex;
   std::vector<Work_item> _work_queue;
   std::atomic_int32_t _work_queue_size;
};

auto get_loader() -> Loader&
{
   static Loader loader;

   return loader;
}

}

void queue_load_lvl(const char* file_name) noexcept
{
   get_loader().queue_load_lvl(file_name);
}

void wait_all_loaded() noexcept
{
   get_loader().wait_all_loaded();
}

}