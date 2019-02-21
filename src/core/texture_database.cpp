#include "texture_database.hpp"
#include "../logger.hpp"

namespace sp::core {

auto Texture_database::get(const std::string& name) const noexcept
   -> Com_ptr<ID3D11ShaderResourceView>
{
   if (auto it = lookup(name); it != _textures.cend()) {
      if (std::holds_alternative<std::weak_ptr<ID3D11ShaderResourceView>>(it->second)) {
         auto ptr =
            std::get<std::weak_ptr<ID3D11ShaderResourceView>>(it->second).lock();

         if (ptr) return Com_ptr<ID3D11ShaderResourceView>{ptr};
      }
      else {
         return std::get<Com_ptr<ID3D11ShaderResourceView>>(it->second);
      }
   }

   log(Log_level::warning, "Texture "sv, std::quoted(name), " does not exist."sv);

   return _default_texture;
}

void Texture_database::add(const std::string& name,
                           std::weak_ptr<ID3D11ShaderResourceView> texture_srv) noexcept
{
   log(Log_level::info, "Loaded texture "sv, std::quoted(name));

   _textures[name] = std::move(texture_srv);
}

void Texture_database::add(const std::string& name,
                           Com_ptr<ID3D11ShaderResourceView> texture_srv) noexcept
{
   log(Log_level::info, "Loaded texture "sv, std::quoted(name));

   _textures[name] = std::move(texture_srv);
}

void Texture_database::clean_lost_textures() noexcept
{
   for (auto it = std::begin(_textures); it != std::cend(_textures);) {
      auto* const weak_srv =
         std::get_if<std::weak_ptr<ID3D11ShaderResourceView>>(&it->second);

      if (!weak_srv) continue;

      if (weak_srv->expired()) {
         log(Log_level::info, "Lost texture "sv, std::quoted(it->first));

         it = _textures.erase(it);
      }
      else {
         ++it;
      }
   }
}

void Texture_database::set_default_texture(Com_ptr<ID3D11ShaderResourceView> texture_srv) noexcept
{
   _default_texture = std::move(texture_srv);
}

auto Texture_database::lookup(const std::string& name) const noexcept
   -> std::unordered_map<std::string, Refernce_types>::const_iterator
{
   if (name.front() == '$') return builtin_lookup(name);

   return _textures.find(name);
}

auto Texture_database::builtin_lookup(const std::string& name) const noexcept
   -> std::unordered_map<std::string, Refernce_types>::const_iterator
{
   Expects(!name.empty());

   using namespace std::literals;

   auto builtin_name = "_SP_BUILTIN_"s;
   builtin_name.append(name.cbegin() + 1, name.cend());

   return _textures.find(builtin_name);
}

}
