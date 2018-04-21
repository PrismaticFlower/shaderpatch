#pragma once

#include "logger.hpp"
#include "texture.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace sp {

class Texture_database {
public:
   Texture get(const std::string& name) const noexcept
   {
      if (_textures.count(name)) {
         auto ptr = _textures.at(name).lock();

         if (ptr) return *ptr;
      }

      log(Log_level::warning, "Texture "sv, std::quoted(name), " does not exist."sv);

      return _default_texture;
   }

   void add(const std::string& name, std::weak_ptr<Texture> texture) noexcept
   {
      _textures[name] = std::move(texture);
   }

   void clean_lost_textures() noexcept
   {
      for (auto it = std::begin(_textures); it != std::cend(_textures);) {

         if (it->second.expired()) {
            it = _textures.erase(it);
         }
         else {
            ++it;
         }
      }
   }

   void set_default_texture(Texture texture)
   {
      _default_texture = std::move(texture);
   }

private:
   std::unordered_map<std::string, std::weak_ptr<Texture>> _textures;
   Texture _default_texture;
};
}
