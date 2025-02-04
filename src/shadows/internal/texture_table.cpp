#include "texture_table.hpp"

#include "../../imgui/imgui.h"

#include <algorithm>

namespace sp::shadows {

void Texture_table::clear() noexcept
{
   _textures.clear();

   _texture_index_from_name.clear();
   _names_from_hash.clear();
}

void Texture_table::add(const std::uint32_t name_hash,
                        const Texture_hash& texture_hash) noexcept
{
   if (!_texture_index_from_name.contains(name_hash)) {
      _texture_index_from_name.emplace(name_hash, _textures.size());

      if (_registered_srvs.contains(texture_hash))
         _textures.push_back(_registered_srvs[texture_hash]);
      else
         _textures.push_back(nullptr);
   }
   else {
      const std::size_t texture_index = _texture_index_from_name[name_hash];

      if (_registered_srvs.contains(texture_hash))
         _textures[texture_index] = _registered_srvs[texture_hash];
   }

   std::vector<std::uint32_t>& name_list = _names_from_hash[texture_hash];

   if (std::ranges::find(name_list, name_hash) == name_list.end()) {
      name_list.push_back(name_hash);
   }
}

void Texture_table::register_(ID3D11ShaderResourceView& srv,
                              const Texture_hash& data_hash) noexcept
{
   _registered_srvs.insert_or_assign(data_hash, copy_raw_com_ptr(srv));
   _registered_hashes.insert_or_assign(&srv, data_hash);

   if (auto names_lookup_it = _names_from_hash.find(data_hash);
       names_lookup_it != _names_from_hash.end()) {
      for (std::uint32_t name : names_lookup_it->second) {
         _textures[_texture_index_from_name[name]] = copy_raw_com_ptr(srv);
      }
   }
}

void Texture_table::unregister(ID3D11ShaderResourceView& srv) noexcept
{
   auto it = _registered_hashes.find(&srv);

   if (it == _registered_hashes.end()) return;

   if (auto names_lookup_it = _names_from_hash.find(it->second);
       names_lookup_it != _names_from_hash.end()) {
      for (std::uint32_t name : names_lookup_it->second) {
         _textures[_texture_index_from_name[name]] = nullptr;
      }
   }

   _registered_srvs.erase(it->second);
   _registered_hashes.erase(it);
}

auto Texture_table::acquire(const std::uint32_t name_hash) noexcept -> std::size_t
{
   auto it = _texture_index_from_name.find(name_hash);

   if (it == _texture_index_from_name.end()) return null_index;

   return it->second;
}

auto Texture_table::get(std::size_t index) const noexcept -> ID3D11ShaderResourceView*
{
   if (index >= _textures.size()) return nullptr;

   return _textures[index].get();
}

auto Texture_table::get_ptr(std::size_t index) const noexcept
   -> ID3D11ShaderResourceView* const*
{
   constexpr static ID3D11ShaderResourceView* null_srvs[1] = {nullptr};

   if (index >= _textures.size()) return null_srvs;

   return _textures[index].get_ptr();
}

auto Texture_table::allocated_bytes() const noexcept -> std::size_t
{
   std::size_t count =
      _textures.capacity() * sizeof(decltype(_textures)::value_type) +

      _texture_index_from_name.capacity() *
         sizeof(decltype(_texture_index_from_name)::value_type) +
      _names_from_hash.capacity() * sizeof(decltype(_names_from_hash)::value_type) +

      _registered_srvs.capacity() * sizeof(decltype(_registered_srvs)::value_type) +
      _registered_hashes.capacity() * sizeof(decltype(_registered_hashes)::value_type);

   for (const auto& [hash, names] : _names_from_hash) {
      count += names.capacity() * sizeof(decltype(names)::value_type);
   }

   return count;
}

void Texture_table::show_imgui_page(const Name_table& name_table) noexcept
{
   (void)name_table;

   static std::uint32_t selected_texture_hash = 0;
   std::uint32_t selected_texture_index = UINT32_MAX;

   if (ImGui::BeginTabBar("Texture Tabs")) {
      if (ImGui::BeginTabItem("Explorer")) {
         if (ImGui::BeginChild("##list", {ImGui::GetContentRegionAvail().x * 0.4f, 0.0f},
                               ImGuiChildFlags_ResizeX | ImGuiChildFlags_FrameStyle)) {
            for (const auto& [texture_name_hash, texture_index] :
                 _texture_index_from_name) {
               const bool selected = selected_texture_hash == texture_name_hash;

               const char* name = name_table.lookup(texture_name_hash);

               if (ImGui::Selectable(name, selected)) {
                  selected_texture_hash = texture_name_hash;
               }

               ImGui::SetItemTooltip(name);

               if (selected) selected_texture_index = texture_index;
            }
         }

         ImGui::EndChild();

         ImGui::SameLine();

         if (ImGui::BeginChild("##texture") &&
             selected_texture_index < _textures.size()) {
            ImGui::SeparatorText("Texture Hashes");

            for (const auto& [texture_hash, texture_names] : _names_from_hash) {
               for (const std::uint32_t name : texture_names) {
                  if (selected_texture_hash == name) {
                     ImGui::Text("%.8x%.8x%.8x%.8x%.8x%.8x%.8x%.8x",
                                 texture_hash.words[0], texture_hash.words[1],
                                 texture_hash.words[2], texture_hash.words[3],
                                 texture_hash.words[4], texture_hash.words[5],
                                 texture_hash.words[6], texture_hash.words[7]);
                  }
               }
            }

            ImGui::SeparatorText("Preview");

            ID3D11ShaderResourceView* shader_resource_view =
               _textures[selected_texture_index].get();

            if (shader_resource_view) {
               Com_ptr<ID3D11Resource> resource;

               shader_resource_view->GetResource(resource.clear_and_assign());

               Com_ptr<ID3D11Texture2D> texture2d;

               resource->QueryInterface(texture2d.clear_and_assign());

               D3D11_TEXTURE2D_DESC desc{};

               texture2d->GetDesc(&desc);

               ImGui::BeginChild("##container",
                                 {
                                    static_cast<float>(desc.Width) +
                                       ImGui::GetStyle().WindowPadding.x * 2.0f,
                                    static_cast<float>(desc.Height) +
                                       ImGui::GetStyle().WindowPadding.y * 2.0f,
                                 });

               const ImVec2 image_bottom_right = {
                  ImGui::GetCursorScreenPos().x + static_cast<float>(desc.Width),
                  ImGui::GetCursorScreenPos().y + static_cast<float>(desc.Height),
               };

               ImGui::GetWindowDrawList()->AddImage(reinterpret_cast<ImTextureID>(
                                                       shader_resource_view),
                                                    ImGui::GetCursorScreenPos(),
                                                    image_bottom_right);

               ImGui::EndChild();
            }
         }

         ImGui::EndChild();

         ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Registered Viewer")) {
         for (const auto& [texture_hash, shader_resource_view] : _registered_srvs) {
            Com_ptr<ID3D11Resource> resource;

            shader_resource_view->GetResource(resource.clear_and_assign());

            Com_ptr<ID3D11Texture2D> texture2d;

            resource->QueryInterface(texture2d.clear_and_assign());

            D3D11_TEXTURE2D_DESC desc{};

            texture2d->GetDesc(&desc);

            ImGui::Image(reinterpret_cast<ImTextureID>(shader_resource_view.get()),
                         {static_cast<float>(desc.Width),
                          static_cast<float>(desc.Height)});

            ImGui::SetItemTooltip("%.8x%.8x%.8x%.8x%.8x%.8x%.8x%.8x",
                                  texture_hash.words[0], texture_hash.words[1],
                                  texture_hash.words[2], texture_hash.words[3],
                                  texture_hash.words[4], texture_hash.words[5],
                                  texture_hash.words[6], texture_hash.words[7]);
         }

         ImGui::EndTabItem();
      }

      ImGui::EndTabBar();
   }
}

}