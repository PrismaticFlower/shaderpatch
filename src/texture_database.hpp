#pragma once

#include "logger.hpp"
#include "texture.hpp"

#include <memory>
#include <string>
#include <unordered_map>

#include <gsl/gsl>

namespace sp {

class Texture_database {
public:
   Texture get(const std::string& name) const noexcept
   {
      if (auto it = lookup(name); it != _textures.cend()) {
         auto ptr = it->second.lock();

         if (ptr) return *ptr;
      }

      log(Log_level::warning, "Texture "sv, std::quoted(name), " does not exist."sv);

      return _default_texture;
   }

   void add(const std::string& name, std::weak_ptr<Texture> texture) noexcept
   {
      log(Log_level::info, "Loaded texture "sv, std::quoted(name));

      _textures[name] = std::move(texture);
   }

   void clean_lost_textures() noexcept
   {
      for (auto it = std::begin(_textures); it != std::cend(_textures);) {

         if (it->second.expired()) {
            log(Log_level::info, "Lost texture "sv, std::quoted(it->first));

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
   using Textures_it =
      std::unordered_map<std::string, std::weak_ptr<Texture>>::const_iterator;

   Textures_it lookup(const std::string& name) const noexcept
   {
      if (name.front() == '$') return builtin_lookup(name);

      return _textures.find(name);
   }

   Textures_it builtin_lookup(const std::string& name) const noexcept
   {
      Expects(!name.empty());

      using namespace std::literals;

      auto builtin_name = "_SP_BUILTIN_"s;
      builtin_name.append(name.cbegin() + 1, name.cend());

      return _textures.find(builtin_name);
   }

   std::unordered_map<std::string, std::weak_ptr<Texture>> _textures;
   Texture _default_texture;
};
}
