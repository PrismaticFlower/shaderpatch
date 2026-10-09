#pragma once

#include <cstdint>

struct ID3D11ShaderResourceView;

namespace sp::game_support {

/// @brief Lookup a texture in the game's texture table.
/// @param name_hash The hash of the texture's name.
/// @param srgb_view Get SP's sRGB view of the texture.
/// @return The ID3D11ShaderResourceView* read belonging to the texture or an empty core::Game_texture.
auto lookup_texture(std::uint32_t name_hash, const bool srgb_view)
   -> ID3D11ShaderResourceView*;

}