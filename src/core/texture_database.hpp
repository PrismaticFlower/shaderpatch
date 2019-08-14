#pragma once

#include "com_ptr.hpp"

#include <memory>
#include <string>
#include <vector>

#include <gsl/gsl>

#include <d3d11_1.h>

namespace sp::core {

class Shader_resource_database {
public:
   auto at_if(const std::string_view name) const noexcept
      -> Com_ptr<ID3D11ShaderResourceView>;

   auto lookup_name(ID3D11ShaderResourceView* srv) -> std::string;

   void insert(Com_ptr<ID3D11ShaderResourceView> texture_srv,
               const std::string_view name) noexcept;

   void erase(ID3D11ShaderResourceView* srv) noexcept;

   auto imgui_resource_picker() noexcept -> ID3D11ShaderResourceView*;

private:
   auto lookup(const std::string_view name) const noexcept
      -> ID3D11ShaderResourceView*;

   auto builtin_lookup(const std::string_view name) const noexcept
      -> ID3D11ShaderResourceView*;

   std::vector<std::pair<Com_ptr<ID3D11ShaderResourceView>, std::string>> _resources = [] {
      std::vector<std::pair<Com_ptr<ID3D11ShaderResourceView>, std::string>> res;
      res.reserve(1024);

      return res;
   }();
   std::string _imgui_filter;
};
}
