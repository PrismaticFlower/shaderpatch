
#include "game_rendertarget.hpp"

namespace sp::core {

Game_rendertarget::Game_rendertarget(ID3D11Device1& device,
                                     const DXGI_FORMAT format, const UINT width,
                                     const UINT height, const UINT sample_count,
                                     Game_rt_flags flags) noexcept
   : format{format},
     width{static_cast<std::uint16_t>(width)},
     height{static_cast<std::uint16_t>(height)},
     sample_count{static_cast<std::uint16_t>(sample_count)},
     flags{flags}
{
   const auto texture_desc =
      CD3D11_TEXTURE2D_DESC{format,
                            width,
                            height,
                            1,
                            1,
                            D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
                            D3D11_USAGE_DEFAULT,
                            0,
                            sample_count,
                            (sample_count != 1)
                               ? static_cast<UINT>(D3D11_STANDARD_MULTISAMPLE_PATTERN)
                               : 0};

   device.CreateTexture2D(&texture_desc, nullptr, texture.clear_and_assign());
   device.CreateRenderTargetView(texture.get(), nullptr, rtv.clear_and_assign());
   device.CreateShaderResourceView(texture.get(), nullptr, srv.clear_and_assign());
}

Game_rendertarget::Game_rendertarget(Com_ptr<ID3D11Texture2D> texture,
                                     Com_ptr<ID3D11RenderTargetView> rtv,
                                     Com_ptr<ID3D11ShaderResourceView> srv,
                                     const DXGI_FORMAT format, const UINT width,
                                     const UINT height, const UINT sample_count,
                                     Game_rt_flags flags) noexcept
   : texture{std::move(texture)},
     rtv{std::move(rtv)},
     srv{std::move(srv)},
     format{format},
     width{static_cast<std::uint16_t>(width)},
     height{static_cast<std::uint16_t>(height)},
     sample_count{static_cast<std::uint16_t>(sample_count)},
     flags{flags}
{
}

}
