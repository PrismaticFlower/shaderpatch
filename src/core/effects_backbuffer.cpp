
#include "effects_backbuffer.hpp"

namespace sp::core {

Effects_backbuffer::Effects_backbuffer(ID3D11Device1& device, const DXGI_FORMAT format,
                                       const UINT width, const UINT height) noexcept
   : texture{[&] {
        Com_ptr<ID3D11Texture2D> init_texture;

        const auto texture_desc =
           CD3D11_TEXTURE2D_DESC{format,
                                 width,
                                 height,
                                 1,
                                 1,
                                 D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET};

        device.CreateTexture2D(&texture_desc, nullptr,
                               init_texture.clear_and_assign());

        return init_texture;
     }()},
     game_rtv{[&] {
        const auto rtv_desc =
           CD3D11_RENDER_TARGET_VIEW_DESC{D3D11_RTV_DIMENSION_TEXTURE2D, format};

        Com_ptr<ID3D11RenderTargetView> init_rtv;

        device.CreateRenderTargetView(texture.get(), &rtv_desc,
                                      init_rtv.clear_and_assign());

        return init_rtv;
     }()},
     game_srv{[&] {
        const auto srv_desc =
           CD3D11_SHADER_RESOURCE_VIEW_DESC{D3D11_SRV_DIMENSION_TEXTURE2D, format};

        Com_ptr<ID3D11ShaderResourceView> init_srv;

        device.CreateShaderResourceView(texture.get(), &srv_desc,
                                        init_srv.clear_and_assign());

        return init_srv;
     }()},
     srv{[&] {
        const auto srv_desc =
           CD3D11_SHADER_RESOURCE_VIEW_DESC{D3D11_SRV_DIMENSION_TEXTURE2D, format};

        Com_ptr<ID3D11ShaderResourceView> init_srv;

        device.CreateShaderResourceView(texture.get(), &srv_desc,
                                        init_srv.clear_and_assign());

        return init_srv;
     }()},
     format{format},
     width{width},
     height{height}
{
}

auto Effects_backbuffer::game_rendertarget() const noexcept -> Game_rendertarget
{
   return {texture, game_rtv, game_srv, nullptr, format, width, height};
}

auto Effects_backbuffer::postprocess_input() const noexcept -> effects::Postprocess_input
{
   return {*srv, format, width, height};
}

}
