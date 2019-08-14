
#include "texture_loader.hpp"
#include "../logger.hpp"
#include "memory_mapped_file.hpp"
#include "patch_texture_io.hpp"

namespace sp::core {

auto load_texture_lvl(const std::filesystem::path lvl_path,
                      ID3D11Device1& device) noexcept -> Shader_resource_database
{
   try {
      win32::Memeory_mapped_file file{lvl_path};
      ucfb::Reader reader{file.bytes()};
      Shader_resource_database database;

      while (reader) {
         auto [srv, name] =
            load_patch_texture(reader.read_child_strict<"sptx"_mn>(), device);

         database.insert(std::move(srv), std::move(name));
      }

      return database;
   }
   catch (std::exception& e) {
      log_and_terminate("Failed to load builtin textures! reason: ", e.what());
   }
}

}
