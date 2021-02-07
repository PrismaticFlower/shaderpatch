#pragma once

#include "../texture_database.hpp"
#include "com_ptr.hpp"

#include <atomic>
#include <cstdint>
#include <future>
#include <memory>
#include <shared_mutex>

#include <d3d11_4.h>

namespace sp::core::text {

struct Freetype_state;

constexpr std::size_t atlas_count = 4;

class Font_atlas_builder {
public:
   Font_atlas_builder(Com_ptr<ID3D11Device5> device) noexcept;

   ~Font_atlas_builder();

   void set_dpi(const std::uint32_t dpi) noexcept;

   bool update_srv_database(Shader_resource_database& database) noexcept;

private:
   void build_atlases(const std::uint32_t dpi) noexcept;

   void build_atlas(const std::size_t atlas_index, const std::uint32_t dpi) noexcept;

   constexpr static DXGI_FORMAT atlas_format = DXGI_FORMAT_A8_UNORM;

   std::uint32_t current_dpi = 0;

   Com_ptr<ID3D11Device5> _device;

   std::shared_mutex _atlas_mutex;
   std::array<Com_ptr<ID3D11Buffer>, atlas_count> _atlas_index_buffer;
   std::array<Com_ptr<ID3D11ShaderResourceView>, atlas_count> _atlas_index_srv;
   std::array<Com_ptr<ID3D11Texture2D>, atlas_count> _atlas_texture;
   std::array<Com_ptr<ID3D11ShaderResourceView>, atlas_count> _atlas_texture_srv;
   std::array<bool, atlas_count> _atlas_dirty{};

   std::unique_ptr<Freetype_state> _freetype_state;
   std::atomic_bool _cancel_build = false;
   std::future<void> _build_atlas_future;
};

}
