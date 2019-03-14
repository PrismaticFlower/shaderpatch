
#include "effects_backbuffer.hpp"

namespace sp::core {

Effects_backbuffer::Effects_backbuffer(ID3D11Device1& device, const DXGI_FORMAT format,
                                       const UINT width, const UINT height,
                                       const UINT sample_count) noexcept
   : texture{[&] {
        Com_ptr<ID3D11Texture2D> init_texture;

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

        device.CreateTexture2D(&texture_desc, nullptr,
                               init_texture.clear_and_assign());

        return init_texture;
     }()},
     rtv{[&] {
        Com_ptr<ID3D11RenderTargetView> init_rtv;

        device.CreateRenderTargetView(texture.get(), nullptr,
                                      init_rtv.clear_and_assign());

        return init_rtv;
     }()},
     srv{[&] {
        Com_ptr<ID3D11ShaderResourceView> init_srv;

        device.CreateShaderResourceView(texture.get(), nullptr,
                                        init_srv.clear_and_assign());

        return init_srv;
     }()},
     format{format},
     width{width},
     height{height},
     sample_count{sample_count}
{
}

auto Effects_backbuffer::game_rendertarget() const noexcept -> Game_rendertarget
{
   return {texture, rtv, srv, format, width, height, sample_count};
}

auto Effects_backbuffer::postprocess_input() const noexcept -> effects::Postprocess_input
{
   return {*srv, format, width, height, sample_count};
}

Effects_backbuffer::operator bool() const noexcept
{
   return texture != nullptr;
}

}
