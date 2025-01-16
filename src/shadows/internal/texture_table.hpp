#pragma once

#include "../texture_hasher.hpp"
#include "name_table.hpp"

#include "com_ptr.hpp"

#include <vector>

#include <absl/container/flat_hash_map.h>

#include <d3d11_2.h>

namespace sp::shadows {

/// @brief Stores a table to translate name hashes to strings for printing/UI.
struct Texture_table {
   const static std::size_t null_index = 0xff'ff'ff'ff;

   /// @brief Clear the table.
   void clear() noexcept;

   /// @brief Add a texture to the table.
   /// @param name_hash The name of the texture.
   /// @param texture_hash The hash of the data of the first subresource in the texture.
   void add(const std::uint32_t name_hash, const Texture_hash& texture_hash) noexcept;

   /// @brief Register an srv with the table.
   /// @param srv The SRV.
   /// @param data_hash The hash of the data of the first subresource in the texture.
   void register_(ID3D11ShaderResourceView& srv, const Texture_hash& data_hash) noexcept;

   /// @brief Unregister an srv with the table. Turning it's binding into a null.
   /// @param srv The srv to unregister.
   void unregister(ID3D11ShaderResourceView& srv) noexcept;

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
   void show_imgui_page(const Name_table& name_table) noexcept;

private:
   std::vector<Com_ptr<ID3D11ShaderResourceView>> _textures;

   absl::flat_hash_map<std::uint32_t, std::size_t> _texture_index_from_name;
   absl::flat_hash_map<Texture_hash, std::vector<std::uint32_t>> _names_from_hash;

   absl::flat_hash_map<Texture_hash, Com_ptr<ID3D11ShaderResourceView>> _registered_srvs;
   absl::flat_hash_map<void*, Texture_hash> _registered_hashes;
};

}