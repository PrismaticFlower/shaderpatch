#include "texture_table.hpp"

#include "../../imgui/imgui.h"

#include "../../game_support/texture_table.hpp"

#include <algorithm>

namespace sp::shadows {

void Texture_table::update_from_game() noexcept
{
   for (std::size_t i = 0; i < _textures.size(); ++i) {
      ID3D11ShaderResourceView* new_srv =
         game_support::lookup_texture(_texture_name_hashes[i], false);

      if (new_srv != _textures[i]) _textures[i] = copy_raw_com_ptr(new_srv);
   }
}

void Texture_table::clear() noexcept
{
   _textures.clear();
   _texture_name_hashes.clear();
}

auto Texture_table::acquire(const std::uint32_t name_hash) noexcept -> std::size_t
{
   auto it = std::find(_texture_name_hashes.begin(), _texture_name_hashes.end(),
                       name_hash);

   if (it == _texture_name_hashes.end()) {
      const std::size_t new_index = _textures.size();

      _textures.emplace_back();
      _texture_name_hashes.push_back(name_hash);

      return new_index;
   }

   return static_cast<std::size_t>(std::distance(_texture_name_hashes.end(), it));
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
   std::size_t count = _textures.capacity() * sizeof(decltype(_textures)::value_type) +
                       _texture_name_hashes.capacity() *
                          sizeof(decltype(_texture_name_hashes)::value_type);

   return count;
}

void Texture_table::show_imgui_page() noexcept
{
   static std::uint32_t selected_texture_hash = 0;
   std::uint32_t selected_texture_index = UINT32_MAX;

   if (ImGui::BeginTabBar("Texture Tabs")) {
      if (ImGui::BeginTabItem("Explorer")) {
         if (ImGui::BeginChild("##list", {ImGui::GetContentRegionAvail().x * 0.4f, 0.0f},
                               ImGuiChildFlags_ResizeX | ImGuiChildFlags_FrameStyle)) {
            for (std::size_t texture_index = 0;
                 texture_index < _texture_name_hashes.size(); ++texture_index) {
               const std::size_t texture_name_hash =
                  _texture_name_hashes[texture_index];
               const bool selected = selected_texture_hash == texture_name_hash;

               ImGui::PushID(texture_name_hash);

               if (ImGui::Selectable("##texture", selected)) {
                  selected_texture_hash = texture_name_hash;
               }

               ImGui::SetItemTooltip("0x%x", texture_name_hash);

               if (selected) selected_texture_index = texture_index;

               ImGui::PopID();
            }
         }

         ImGui::EndChild();

         ImGui::SameLine();

         if (ImGui::BeginChild("##texture") &&
             selected_texture_index < _textures.size()) {
            ImGui::SeparatorText("Texture Hashes");

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

      ImGui::EndTabBar();
   }
}

}