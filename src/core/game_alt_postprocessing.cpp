
#include "game_alt_postprocessing.hpp"

namespace sp::core {

namespace {

constexpr auto postprocess_target_width = 480;
constexpr auto postprocess_target_height = 270;

auto select_mip_level(const Game_rendertarget& dest) -> int
{
   constexpr auto target_pixel_count =
      postprocess_target_width * postprocess_target_height;

   const std::array<int, 16> mip_pixels_distance = [&] {
      std::array<int, 16> mip_pixels_distance;

      std::iota(mip_pixels_distance.begin(), mip_pixels_distance.end(), 0);

      for (auto& i : mip_pixels_distance) {
         i = std::abs(target_pixel_count - (dest.width >> i) * (dest.height >> i));
      }

      return mip_pixels_distance;
   }();

   std::array levels{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

   std::sort(levels.begin(), levels.end(), [&](const int l, const int r) {
      return mip_pixels_distance[l] < mip_pixels_distance[r];
   });

   return levels[0];
}

}

Game_alt_postprocessing::Game_alt_postprocessing(ID3D11Device5& device,
                                                 shader::Database& shaders) noexcept
   : _device{copy_raw_com_ptr(device)},
     _scene_blur{device, shaders},
     _scope_blur{device, shaders},
     _bloom{device, shaders}
{
}

void Game_alt_postprocessing::blur_factor(const float factor) noexcept
{
   _scene_blur.blur_factor(factor);
}

void Game_alt_postprocessing::bloom_threshold(const float threshold) noexcept
{
   _bloom.threshold(threshold);
}

void Game_alt_postprocessing::bloom_intensity(const float intensity) noexcept
{
   _bloom.intensity(intensity);
}

void Game_alt_postprocessing::apply_scene_blur(ID3D11DeviceContext4& dc,
                                               const Game_rendertarget& src_dest,
                                               effects::Rendertarget_allocator& rt_allocator) noexcept
{
   update_texture(dc, src_dest);

   _scene_blur.apply(dc, src_dest, *_lowest_mip_srv, _lowest_mip_width,
                     _lowest_mip_height, rt_allocator);
}

void Game_alt_postprocessing::apply_scope_blur(ID3D11DeviceContext4& dc,
                                               const Game_rendertarget& src_dest,
                                               effects::Rendertarget_allocator& rt_allocator) noexcept
{
   update_texture(dc, src_dest);

   _scope_blur.apply(dc, src_dest, *_lowest_mip_srv, _lowest_mip_width,
                     _lowest_mip_height, rt_allocator);
}

void Game_alt_postprocessing::apply_bloom(ID3D11DeviceContext4& dc,
                                          const Game_rendertarget& src_dest,
                                          effects::Rendertarget_allocator& rt_allocator) noexcept
{
   update_texture(dc, src_dest);

   _bloom.apply(dc, src_dest, *_mips_srv, rt_allocator);
}

void Game_alt_postprocessing::end_frame() noexcept
{
   _generated_mips = false;
}

void Game_alt_postprocessing::update_texture(ID3D11DeviceContext4& dc,
                                             const Game_rendertarget& game_texture) noexcept
{
   if (_game_texture != game_texture.texture) {
      _game_texture = game_texture.texture;

      const UINT mip_level = select_mip_level(game_texture);

      const CD3D11_TEXTURE2D_DESC desc{game_texture.format,
                                       game_texture.width,
                                       game_texture.height,
                                       1,
                                       mip_level + 1,
                                       D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
                                       D3D11_USAGE_DEFAULT,
                                       0,
                                       1,
                                       0,
                                       D3D11_RESOURCE_MISC_GENERATE_MIPS};

      const CD3D11_SHADER_RESOURCE_VIEW_DESC lowest_mip_srv_desc{D3D11_SRV_DIMENSION_TEXTURE2D,
                                                                 DXGI_FORMAT_UNKNOWN,
                                                                 mip_level, 1};
      const CD3D11_RENDER_TARGET_VIEW_DESC rtv_desc{D3D11_RTV_DIMENSION_TEXTURE2D,
                                                    DXGI_FORMAT_UNKNOWN,
                                                    mip_level, 1};

      _device->CreateTexture2D(&desc, nullptr, _mips_texture.clear_and_assign());
      _device->CreateShaderResourceView(_mips_texture.get(), nullptr,
                                        _mips_srv.clear_and_assign());
      _device->CreateShaderResourceView(_mips_texture.get(), &lowest_mip_srv_desc,
                                        _lowest_mip_srv.clear_and_assign());

      _lowest_mip_width = game_texture.width >> mip_level;
      _lowest_mip_height = game_texture.height >> mip_level;
   }

   if (!std::exchange(_generated_mips, true)) {
      if (game_texture.sample_count > 1) {
         dc.ResolveSubresource(_mips_texture.get(), 0, _game_texture.get(), 0,
                               game_texture.format);
      }
      else {
         dc.CopySubresourceRegion(_mips_texture.get(), 0, 0, 0, 0,
                                  _game_texture.get(), 0, nullptr);
      }

      dc.GenerateMips(_mips_srv.get());
   }
}

}
