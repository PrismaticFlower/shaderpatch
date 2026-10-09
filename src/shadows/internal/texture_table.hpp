#pragma once

#include "com_ptr.hpp"

#include <vector>

#include <d3d11_2.h>

namespace sp::shadows {

struct Texture_table {
   const static std::size_t null_index = 0xff'ff'ff'ff;

   /// @brief Update the table using the game's one.
   void update_from_game() noexcept;

   /// @brief Clear the table.
   void clear() noexcept;

   /// @brief Acquire the index to a texture from a name hash.
   /// @param name_hash The name of the texture.
   /// @return An index that can be passed to get or get_ptr.
   auto acquire(const std::uint32_t name_hash) noexcept -> std::size_t;

   /// @brief Gets a texture from an index.
   /// @param index The index of the texture to get.
   /// @return The SRV for the texture or nullptr if index is invalid.
   auto get(std::size_t index) const noexcept -> ID3D11ShaderResourceView*;

   /// @brief Gets a texture from an index.
   /// @param index The index of the texture to get.
   /// @return The SRV for the texture or nullptr if index is invalid.
   auto get_ptr(std::size_t index) const noexcept -> ID3D11ShaderResourceView* const*;

   /// @brief Returns the approximate count for how much CPU memory has been allocated by the Texture_table.
   /// @return The approximate count of allocated bytes.
   auto allocated_bytes() const noexcept -> std::size_t;

   /// @brief Shows a Dear ImGui "page" of the table. (As in this won't create window rather it'll just submit it's contents)
   void show_imgui_page() noexcept;

private:
   std::vector<Com_ptr<ID3D11ShaderResourceView>> _textures;
   std::vector<std::uint32_t> _texture_name_hashes;
};

}