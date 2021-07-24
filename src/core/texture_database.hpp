#pragma once

#include "com_ptr.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include <gsl/gsl>

#include <d3d11_1.h>

namespace sp::core {

class Shader_resource_database {
public:
   struct Imgui_pick_result {
      ID3D11ShaderResourceView* srv = nullptr;
      std::string_view name;
   };

   struct Reverse_lookup_result {
      bool found = false;
      std::string_view name;
   };

   auto at_if(const std::string_view name) const noexcept
      -> Com_ptr<ID3D11ShaderResourceView>;

   auto reverse_lookup(ID3D11ShaderResourceView* srv) noexcept -> Reverse_lookup_result;

   void insert(Com_ptr<ID3D11ShaderResourceView> texture_srv,
               const std::string_view name) noexcept;

   void erase(ID3D11ShaderResourceView* srv) noexcept;

   auto imgui_resource_picker() noexcept -> Imgui_pick_result;

   auto size() const noexcept -> std::size_t
   {
      std::scoped_lock lock{_resources_mutex};

      return _resources.size();
   }

private:
   auto lookup(const std::string_view name) const noexcept
      -> ID3D11ShaderResourceView*;

   auto builtin_lookup(const std::string_view name) const noexcept
      -> ID3D11ShaderResourceView*;

   mutable std::mutex _resources_mutex;
   std::vector<std::pair<Com_ptr<ID3D11ShaderResourceView>, std::string>> _resources = [] {
      std::vector<std::pair<Com_ptr<ID3D11ShaderResourceView>, std::string>> res;
      res.reserve(1024);

      return res;
   }();
   std::string _imgui_filter;
};
}
