
#include "font_atlas_builder.hpp"
#include "../../freetype_helpers.hpp"
#include "../../game_support/font_info.hpp"
#include "../../logger.hpp"
#include "../../user_config.hpp"
#include "../../windows_fonts_folder.hpp"
#include "utility.hpp"

#include <algorithm>
#include <array>
#include <execution>
#include <filesystem>
#include <numeric>
#include <optional>
#include <span>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H

using namespace std::literals;

namespace sp::core::text {

namespace {

using game_support::atlas_font_sizes;
using game_support::atlas_index_names;
using game_support::atlas_names;
using game_support::game_glyphs;
using game_support::glyph_count;

constexpr std::uint32_t base_dpi = 96;

struct Rendered_glyph {
   std::uint32_t width;
   std::uint32_t height;
   std::vector<std::byte> data;
};

struct Glyph_location {
   float left;
   float right;
   float top;
   float bottom;
};

static_assert(sizeof(Glyph_location) == 16);

auto copy_freetype_bitmap(FT_Bitmap bitmap) -> Rendered_glyph
{
   Rendered_glyph glyph{.width = bitmap.width, .height = bitmap.rows};

   glyph.data.resize(bitmap.width * bitmap.rows);

   for (std::size_t i = 0; i < bitmap.rows; ++i) {
      std::memcpy(glyph.data.data() + i * bitmap.width,
                  bitmap.buffer + i * bitmap.pitch, bitmap.width);
   }

   return glyph;
}

struct Pack_target {
   std::uint32_t pitch;
   std::span<std::byte> data;
   float width;
   float height;
   std::span<Glyph_location, glyph_count> locations;
};

auto pack_glyphs(const std::array<Rendered_glyph, glyph_count>& rendered_glyphs,
                 std::optional<Pack_target> output)
   -> std::pair<std::uint32_t, std::uint32_t>
{
   static_assert(glyph_count <= std::numeric_limits<std::uint8_t>::max());
   std::array<std::uint8_t, glyph_count> pack_order;

   std::iota(pack_order.begin(), pack_order.end(), std::uint8_t{0});

   std::ranges::sort(pack_order, [&](const std::uint8_t l, const std::uint8_t r) {
      return rendered_glyphs[l].height < rendered_glyphs[r].height;
   });

   constexpr std::uint32_t atlas_max_width = 8192;
   constexpr std::uint32_t atlas_max_height = 8192;

   std::uint32_t max_row_height = 0;
   std::uint32_t pack_head_x = 0;
   std::uint32_t pack_head_y = 0;
   bool multiple_rows = false;

   for (auto i : pack_order) {
      auto& glyph = rendered_glyphs[i];

      if ((pack_head_x + glyph.width) >= atlas_max_width) {
         if ((pack_head_y += (std::exchange(max_row_height, 0) + 1)) >= atlas_max_height) {
            log_and_terminate("Required font atlas is too big!"sv);
         }

         multiple_rows = true;
         pack_head_x = 0;
      }

      if (output) {
         for (std::uint32_t y = 0; y < glyph.height; ++y) {
            auto* const dest_row = &output->data[(pack_head_y + y) * output->pitch];
            auto* const dest = dest_row + pack_head_x;
            auto* const src = glyph.data.data() + (y * glyph.width);

            std::memcpy(dest, src, glyph.width);
         }

         const float atlas_left = (pack_head_x + 0.5f) / output->width;
         const float atlas_right = (pack_head_x + glyph.width + 0.5f) / output->width;
         const float atlas_top = (pack_head_y + 0.5f) / output->height;
         const float atlas_bottom =
            (pack_head_y + glyph.height + 0.5f) / output->height;

         output->locations[i] = {.left = atlas_left,
                                 .right = atlas_right,
                                 .top = atlas_top,
                                 .bottom = atlas_bottom};
      }

      pack_head_x += glyph.width + 1;
      max_row_height = std::max(glyph.height, max_row_height);
   }

   if ((pack_head_y += max_row_height) >= atlas_max_height) {
      log_and_terminate("Required font atlas is too big!"sv);
   }

   const std::uint32_t atlas_width =
      multiple_rows ? atlas_max_width : (pack_head_x - 1);
   const std::uint32_t atlas_height = pack_head_y;

   return {atlas_width, atlas_height};
}

}

struct Freetype_state {
   explicit Freetype_state(std::vector<FT_Byte> font_data) noexcept
      : font_data{std::move(font_data)}
   {
      for (auto& face : faces) {
         face = make_freetype_face(library, this->font_data);
      }
   }

   std::vector<FT_Byte> font_data;
   Freetype_ptr<FT_Library, FT_Done_FreeType> library = make_freetype_library();
   std::array<Freetype_ptr<FT_Face, FT_Done_Face>, atlas_count> faces;
};

Font_atlas_builder::Font_atlas_builder(Com_ptr<ID3D11Device5> device) noexcept
   : _device{device}
{
   _freetype_state = std::make_unique<Freetype_state>(load_font_data(
      windows_fonts_folder() / user_config.developer.scalable_font_name));
}

Font_atlas_builder::~Font_atlas_builder()
{
   _cancel_build = true;
}

void Font_atlas_builder::set_dpi(const std::uint32_t dpi) noexcept
{
   if (std::exchange(current_dpi, dpi) == dpi) return;

   if (_build_atlas_future.valid()) {
      _cancel_build = true;
      _build_atlas_future.wait();
   }

   _cancel_build = false;
   _build_atlas_future =
      std::async(std::launch::async, &Font_atlas_builder::build_atlases, this, dpi);
}

bool Font_atlas_builder::update_srv_database(Shader_resource_database& database) noexcept
{
   std::shared_lock lock{_atlas_mutex};

   bool updated = false;

   for (std::size_t i = 0; i < atlas_count; ++i) {
      if (!_atlas_dirty[i].exchange(false)) continue;

      database.insert(_atlas_index_srv[i], atlas_index_names[i]);
      database.insert(_atlas_texture_srv[i], atlas_names[i]);

      updated = true;
   }

   return updated;
}

bool Font_atlas_builder::use_scalable_fonts() noexcept
{
   const static bool use =
      user_config.display.scalable_fonts &&
      std::filesystem::exists(windows_fonts_folder() /
                              user_config.developer.scalable_font_name);

   return use;
}

void Font_atlas_builder::build_atlases(const std::uint32_t dpi) noexcept
{
   std::for_each_n(std::execution::par, Index_iterator{}, atlas_count,
                   [this, dpi](std::size_t index) { build_atlas(index, dpi); });
}

void Font_atlas_builder::build_atlas(const std::size_t atlas_index,
                                     const std::uint32_t dpi) noexcept
{
   FT_Face face = _freetype_state->faces[atlas_index].get();

   FT_Set_Pixel_Sizes(face, 0, atlas_font_sizes[atlas_index] * dpi / base_dpi);

   std::array<Rendered_glyph, glyph_count> rendered_glyphs;

   for (std::size_t i = 0; i < rendered_glyphs.size(); ++i) {
      if (_cancel_build.load(std::memory_order_relaxed)) return;

      auto glyph_index = FT_Get_Char_Index(face, game_glyphs[i]);

      freetype_checked_call(FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_BITMAP));

      freetype_checked_call(FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL));

      auto& bitmap = face->glyph->bitmap;

      rendered_glyphs[i] = copy_freetype_bitmap(bitmap);
   }

   const auto [atlas_width, atlas_height] = pack_glyphs(rendered_glyphs, std::nullopt);

   std::vector<std::byte> atlas_data;
   atlas_data.resize(atlas_width * atlas_height);

   std::array<Glyph_location, glyph_count> atlas_locations;

   pack_glyphs(rendered_glyphs,
               Pack_target{.pitch = atlas_width,
                           .data = atlas_data,
                           .width = static_cast<float>(atlas_width),
                           .height = static_cast<float>(atlas_height),
                           .locations = atlas_locations});

   if (_cancel_build.load(std::memory_order_relaxed)) return;

   const D3D11_BUFFER_DESC buffer_desc{.ByteWidth = sizeof(atlas_locations),
                                       .Usage = D3D11_USAGE_IMMUTABLE,
                                       .BindFlags = D3D11_BIND_SHADER_RESOURCE,
                                       .MiscFlags =
                                          D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS};
   const D3D11_SUBRESOURCE_DATA init_buffer_data{.pSysMem = atlas_locations.data(),
                                                 .SysMemPitch = sizeof(atlas_locations),
                                                 .SysMemSlicePitch =
                                                    sizeof(atlas_locations)};

   Com_ptr<ID3D11Buffer> atlas_index_buffer;
   if (FAILED(_device->CreateBuffer(&buffer_desc, &init_buffer_data,
                                    atlas_index_buffer.clear_and_assign()))) {
      log_and_terminate("Failed to create buffer for font atlas index!"sv);
   }

   const D3D11_SHADER_RESOURCE_VIEW_DESC atlas_index_srv_desc{
      .Format = DXGI_FORMAT_R32_TYPELESS,
      .ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX,
      .BufferEx = {.FirstElement = 0,
                   .NumElements = sizeof(atlas_locations) / sizeof(std::uint32_t),
                   .Flags = D3D11_BUFFEREX_SRV_FLAG_RAW}};

   Com_ptr<ID3D11ShaderResourceView> atlas_index_srv;
   if (FAILED(_device->CreateShaderResourceView(atlas_index_buffer.get(), &atlas_index_srv_desc,
                                                atlas_index_srv.clear_and_assign()))) {
      log_and_terminate("Failed to create SRV for font atlas index!"sv);
   }

   const D3D11_TEXTURE2D_DESC texture_desc{.Width = atlas_width,
                                           .Height = atlas_height,
                                           .MipLevels = 1,
                                           .ArraySize = 1,
                                           .Format = atlas_format,
                                           .SampleDesc = {1, 0},
                                           .Usage = D3D11_USAGE_IMMUTABLE,
                                           .BindFlags = D3D11_BIND_SHADER_RESOURCE};
   const D3D11_SUBRESOURCE_DATA init_texture_data{.pSysMem = atlas_data.data(),
                                                  .SysMemPitch = atlas_width,
                                                  .SysMemSlicePitch =
                                                     atlas_width * atlas_height};

   Com_ptr<ID3D11Texture2D> atlas_texture;
   if (FAILED(_device->CreateTexture2D(&texture_desc, &init_texture_data,
                                       atlas_texture.clear_and_assign()))) {
      log_and_terminate("Failed to create texture for font atlas!"sv);
   }

   Com_ptr<ID3D11ShaderResourceView> atlas_texture_srv;
   if (FAILED(_device->CreateShaderResourceView(atlas_texture.get(), nullptr,
                                                atlas_texture_srv.clear_and_assign()))) {
      log_and_terminate("Failed to create SRV for font atlas!"sv);
   }

   std::scoped_lock lock{_atlas_mutex};

   _atlas_index_buffer[atlas_index] = atlas_index_buffer;
   _atlas_index_srv[atlas_index] = atlas_index_srv;
   _atlas_texture[atlas_index] = atlas_texture;
   _atlas_texture_srv[atlas_index] = atlas_texture_srv;
   _atlas_dirty[atlas_index].store(true);
}

}
