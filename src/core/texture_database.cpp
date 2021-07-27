#include "texture_database.hpp"
#include "../imgui/imgui_ext.hpp"
#include "../logger.hpp"
#include "string_utilities.hpp"

#include <sstream>

#include "../imgui/imgui.h"

using namespace std::literals;

namespace sp::core {

namespace {

auto unknown_resource_name(ID3D11ShaderResourceView& srv) noexcept -> std::string
{
   D3D11_SHADER_RESOURCE_VIEW_DESC desc{};
   srv.GetDesc(&desc);

   std::ostringstream stream;

   switch (desc.ViewDimension) {
   case D3D11_SRV_DIMENSION_UNKNOWN:
      stream << "Unknown "sv;
      break;
   case D3D11_SRV_DIMENSION_BUFFER:
      stream << "Buffer "sv;
      break;
   case D3D11_SRV_DIMENSION_TEXTURE1D:
      stream << "Texture1D "sv;
      break;
   case D3D11_SRV_DIMENSION_TEXTURE1DARRAY:
      stream << "Texture1DArray "sv;
      break;
   case D3D11_SRV_DIMENSION_TEXTURE2D:
      stream << "Texture2D "sv;
      break;
   case D3D11_SRV_DIMENSION_TEXTURE2DARRAY:
      stream << "Texture2DArray "sv;
      break;
   case D3D11_SRV_DIMENSION_TEXTURE2DMS:
      stream << "Texture2DMS "sv;
      break;
   case D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY:
      stream << "Texture2DMSArray "sv;
      break;
   case D3D11_SRV_DIMENSION_TEXTURE3D:
      stream << "Texture3D "sv;
      break;
   case D3D11_SRV_DIMENSION_TEXTURECUBE:
      stream << "TextureCube "sv;
      break;
   case D3D11_SRV_DIMENSION_TEXTURECUBEARRAY:
      stream << "TextureCubeArray "sv;
      break;
   case D3D11_SRV_DIMENSION_BUFFEREX:
      stream << "BufferEx "sv;
      break;
   }

   stream << &srv;

   return stream.str();
}
}

auto Shader_resource_database::at_if(const std::string_view name) const noexcept
   -> Com_ptr<ID3D11ShaderResourceView>
{
   std::scoped_lock lock{_resources_mutex};

   if (auto srv = lookup(name); srv) {
      return copy_raw_com_ptr(srv);
   }

   return nullptr;
}

auto Shader_resource_database::reverse_lookup(ID3D11ShaderResourceView* srv) noexcept
   -> Reverse_lookup_result
{
   std::scoped_lock lock{_resources_mutex};

   auto it = std::find_if(_resources.cbegin(), _resources.cend(),
                          [srv](const auto& res) { return res.first == srv; });

   if (it == _resources.cend()) {
      return {.found = false};
   }

   return {.found = true, .name = it->second};
}

void Shader_resource_database::insert(Com_ptr<ID3D11ShaderResourceView> srv,
                                      const std::string_view name) noexcept
{
   std::scoped_lock lock{_resources_mutex};

   std::string name_str{name.empty() ? unknown_resource_name(*srv) : std::string{name}};

   if (auto it =
          std::find_if(_resources.begin(), _resources.end(),
                       [&](const auto& res) { return res.second == name_str; });
       it != _resources.end()) {
      it->first = std::move(srv);

      return;
   }

   _resources.emplace_back(std::move(srv), std::move(name_str));
}

void Shader_resource_database::erase(ID3D11ShaderResourceView* srv) noexcept
{
   std::scoped_lock lock{_resources_mutex};

   auto it = std::find_if(_resources.cbegin(), _resources.cend(),
                          [srv](const auto& res) { return res.first == srv; });

   if (it == _resources.cend()) {
      log_and_terminate("Attempt to erase shader resource not present in database!"sv);
   }

   _resources.erase(it);
}

auto Shader_resource_database::imgui_resource_picker() noexcept -> Imgui_pick_result
{
   std::scoped_lock lock{_resources_mutex};

   Imgui_pick_result result{};

   ImGui::InputText("Filter", _imgui_filter);

   ImGui::BeginChild("Resource List", {400.f, 64.0f * 10.0f});

   for (const auto& res : _resources) {
      if (!_imgui_filter.empty() && !contains(res.second, _imgui_filter))
         continue;

      auto* srv = res.first.get();

      D3D11_SHADER_RESOURCE_VIEW_DESC desc{};
      srv->GetDesc(&desc);

      if (desc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE2D ||
          desc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE2DARRAY) {
         if (ImGui::ImageButton(srv, {64, 64})) {
            result = {.srv = srv, .name = res.second};
            break;
         }

         ImGui::SameLine();
         ImGui::Text(res.second.c_str());
      }
      else {
         if (ImGui::Button(res.second.c_str())) {
            result = {.srv = srv, .name = res.second};
            break;
         }
      }
   }

   ImGui::EndChild();

   return result;
}

auto Shader_resource_database::lookup(const std::string_view name) const noexcept
   -> ID3D11ShaderResourceView*
{
   if (name.front() == '$') return builtin_lookup(name);

   auto it = std::find_if(_resources.cbegin(), _resources.cend(),
                          [name](const auto& res) { return res.second == name; });

   return (it != _resources.cend()) ? it->first.get() : nullptr;
}

auto Shader_resource_database::builtin_lookup(const std::string_view name) const noexcept
   -> ID3D11ShaderResourceView*
{
   Expects(!name.empty());

   using namespace std::literals;

   auto builtin_name = "_SP_BUILTIN_"s;
   builtin_name.append(name.cbegin() + 1, name.cend());

   return lookup(builtin_name);
}
}
