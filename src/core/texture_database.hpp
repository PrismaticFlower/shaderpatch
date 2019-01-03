#pragma once

#include "com_ptr.hpp"

#include <memory>
#include <string>
#include <unordered_map>

#include <gsl/gsl>

#include <d3d11_1.h>

namespace sp::core {

class Texture_database {
public:
   auto get(const std::string& name) const noexcept
      -> Com_ptr<ID3D11ShaderResourceView>;

   void add(const std::string& name,
            std::weak_ptr<ID3D11ShaderResourceView> texture_srv) noexcept;

   void clean_lost_textures() noexcept;

   void set_default_texture(Com_ptr<ID3D11ShaderResourceView> texture_srv) noexcept;

private:
   auto lookup(const std::string& name) const noexcept
      -> std::unordered_map<std::string, std::weak_ptr<ID3D11ShaderResourceView>>::const_iterator;

   auto builtin_lookup(const std::string& name) const noexcept
      -> std::unordered_map<std::string, std::weak_ptr<ID3D11ShaderResourceView>>::const_iterator;

   std::unordered_map<std::string, std::weak_ptr<ID3D11ShaderResourceView>> _textures;
   Com_ptr<ID3D11ShaderResourceView> _default_texture;
};
}
