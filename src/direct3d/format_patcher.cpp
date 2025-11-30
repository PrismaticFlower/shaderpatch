
#include "format_patcher.hpp"
#include "../logger.hpp"
#include "upload_scratch_buffer.hpp"
#include "utility.hpp"

#include <array>
#include <execution>
#include <span>

#include <glm/glm.hpp>

#include <comdef.h>

namespace sp::d3d9 {

namespace {

Upload_scratch_buffer patchup_scratch_buffer{524288u, 524288u};

class Format_patcher_l8 final : public Format_patcher {
public:
   auto patch_texture(const DXGI_FORMAT format, const UINT width, const UINT height,
                      const UINT mip_levels, const UINT array_size,
                      Upload_texture& input_texture) const noexcept
      -> std::pair<DXGI_FORMAT, std::unique_ptr<Upload_texture>> override
   {
      Expects(format == DXGI_FORMAT_R8_UNORM);

      auto patched_texture =
         std::make_unique<Upload_texture>(patchup_scratch_buffer,
                                          DXGI_FORMAT_R8G8B8A8_UNORM, width,
                                          height, 1, mip_levels, array_size);

      for (auto index = 0; index < array_size; ++index) {
         for (auto mip = 0; mip < mip_levels; ++mip) {
            patch_subimage(glm::max(width >> mip, 1u), glm::max(height >> mip, 1u),
                           input_texture.subresource(mip, index),
                           patched_texture->subresource(mip, index));
         }
      }

      return {DXGI_FORMAT_R8G8B8A8_UNORM, std::move(patched_texture)};
   }

   auto map_dynamic_texture(const DXGI_FORMAT format, const UINT width,
                            const UINT height, const UINT mip_level) noexcept
      -> core::Mapped_texture override
   {
      Expects(format == DXGI_FORMAT_R8_UNORM);
      Expects(!is_mapped());

      _dynamic_width = glm::max(width >> mip_level, 1u);
      _dynamic_height = glm::max(height >> mip_level, 1u);

      _dynamic_patch_texture =
         std::make_unique<Upload_texture>(patchup_scratch_buffer, format,
                                          _dynamic_width, _dynamic_height, 1, 1, 1);

      return _dynamic_patch_texture->subresource(0, 0);
   }

   void unmap_dynamic_texture(core::Shader_patch& shader_patch,
                              const core::Game_texture& texture,
                              const UINT mip_level) noexcept override
   {
      Expects(is_mapped());

      auto dest = shader_patch.map_dynamic_texture(texture, mip_level,
                                                   D3D11_MAP_WRITE_DISCARD);

      patch_subimage(_dynamic_width, _dynamic_height,
                     _dynamic_patch_texture->subresource(0, 0), dest);

      _dynamic_patch_texture = nullptr;
      shader_patch.unmap_dynamic_texture(texture, mip_level);
   }

   bool is_mapped() const noexcept override
   {
      return _dynamic_patch_texture != nullptr;
   }

private:
   static void patch_subimage(const UINT width, const UINT height,
                              core::Mapped_texture source,
                              core::Mapped_texture dest) noexcept
   {
      std::for_each_n(std::execution::par_unseq, Index_iterator{}, height, [&](const int y) {
         const auto src_row =
            std::span{source.data + (source.row_pitch * y), source.row_pitch};
         auto dest_row = std::span{dest.data + (dest.row_pitch * y), dest.row_pitch};

         for (auto x = 0; x < width; ++x) {
            auto dest_pixel = dest_row.subspan(4 * x, 4);

            dest_pixel[0] = dest_pixel[1] = dest_pixel[2] = src_row[x];
            dest_pixel[3] = std::byte{0xffu};
         }
      });
   }

   UINT _dynamic_width;
   UINT _dynamic_height;
   std::unique_ptr<Upload_texture> _dynamic_patch_texture;
};

class Format_patcher_a8l8 final : public Format_patcher {
public:
   auto patch_texture(const DXGI_FORMAT format, const UINT width, const UINT height,
                      const UINT mip_levels, const UINT array_size,
                      Upload_texture& input_texture) const noexcept
      -> std::pair<DXGI_FORMAT, std::unique_ptr<Upload_texture>> override
   {
      Expects(format == DXGI_FORMAT_R8G8_UNORM);

      auto patched_texture =
         std::make_unique<Upload_texture>(patchup_scratch_buffer,
                                          DXGI_FORMAT_R8G8B8A8_UNORM, width,
                                          height, 1, mip_levels, array_size);

      for (auto index = 0; index < array_size; ++index) {
         for (auto mip = 0; mip < mip_levels; ++mip) {
            patch_subimage(glm::max(width >> mip, 1u), glm::max(height >> mip, 1u),
                           input_texture.subresource(mip, index),
                           patched_texture->subresource(mip, index));
         }
      }

      return {DXGI_FORMAT_R8G8B8A8_UNORM, std::move(patched_texture)};
   }

   auto map_dynamic_texture(const DXGI_FORMAT format, const UINT width,
                            const UINT height, const UINT mip_level) noexcept
      -> core::Mapped_texture override
   {
      Expects(format == DXGI_FORMAT_R8G8_UNORM);
      Expects(!is_mapped());

      _dynamic_width = glm::max(width >> mip_level, 1u);
      _dynamic_height = glm::max(height >> mip_level, 1u);

      _dynamic_patch_texture =
         std::make_unique<Upload_texture>(patchup_scratch_buffer, format,
                                          _dynamic_width, _dynamic_height, 1, 1, 1);

      return _dynamic_patch_texture->subresource(0, 0);
   }

   void unmap_dynamic_texture(core::Shader_patch& shader_patch,
                              const core::Game_texture& texture,
                              const UINT mip_level) noexcept override
   {
      Expects(is_mapped());

      auto dest = shader_patch.map_dynamic_texture(texture, mip_level,
                                                   D3D11_MAP_WRITE_DISCARD);

      patch_subimage(_dynamic_width, _dynamic_height,
                     _dynamic_patch_texture->subresource(0, 0), dest);

      _dynamic_patch_texture = nullptr;
      shader_patch.unmap_dynamic_texture(texture, mip_level);
   }

   bool is_mapped() const noexcept override
   {
      return _dynamic_patch_texture != nullptr;
   }

private:
   static void patch_subimage(const UINT width, const UINT height,
                              core::Mapped_texture source,
                              core::Mapped_texture dest) noexcept
   {
      std::for_each_n(std::execution::par_unseq, Index_iterator{}, height, [&](const int y) {
         const auto src_row =
            std::span{source.data + (source.row_pitch * y), source.row_pitch};
         auto dest_row = std::span{dest.data + (dest.row_pitch * y), dest.row_pitch};

         for (auto x = 0; x < width; ++x) {
            dest_row[x * 4] = src_row[x * 2];
            dest_row[x * 4 + 1] = src_row[x * 2];
            dest_row[x * 4 + 2] = src_row[x * 2];
            dest_row[x * 4 + 3] = src_row[x * 2 + 1];
         }
      });
   }

   UINT _dynamic_width;
   UINT _dynamic_height;
   std::unique_ptr<Upload_texture> _dynamic_patch_texture;
};

}

auto make_l8_format_patcher() noexcept -> std::unique_ptr<Format_patcher>
{
   return std::make_unique<Format_patcher_l8>();
}

auto make_a8l8_format_patcher() noexcept -> std::unique_ptr<Format_patcher>
{
   return std::make_unique<Format_patcher_a8l8>();
}

}
