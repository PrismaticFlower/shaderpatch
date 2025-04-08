
#include "basic_builtin_textures.hpp"
#include "../logger.hpp"
#include "com_ptr.hpp"

#include <cstdint>

namespace sp::core {

namespace {

constexpr auto pack_u32colour(const std::array<std::uint8_t, 4> colour) noexcept
   -> std::uint32_t
{
   std::uint32_t u32colour{};

   u32colour |= colour[0];
   u32colour |= (colour[1] << 8);
   u32colour |= (colour[2] << 16);
   u32colour |= (colour[3] << 24);

   return u32colour;
}

auto make_simple_texture(ID3D11Device5& device,
                         const std::array<std::uint8_t, 4> colour) noexcept
{
   const auto u32col = pack_u32colour(colour);

   constexpr D3D11_TEXTURE2D_DESC desc{.Width = 1,
                                       .Height = 1,
                                       .MipLevels = 1,
                                       .ArraySize = 1,
                                       .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                                       .SampleDesc = {1, 0},
                                       .Usage = D3D11_USAGE_IMMUTABLE,
                                       .BindFlags = D3D11_BIND_SHADER_RESOURCE,
                                       .CPUAccessFlags = 0,
                                       .MiscFlags = 0};
   const D3D11_SUBRESOURCE_DATA data{.pSysMem = &u32col,
                                     .SysMemPitch = sizeof(u32col),
                                     .SysMemSlicePitch = sizeof(u32col)};

   Com_ptr<ID3D11Texture2D> tex;
   if (FAILED(device.CreateTexture2D(&desc, &data, tex.clear_and_assign()))) {
      log_and_terminate("Failed to create texture for builtin!");
   }

   Com_ptr<ID3D11ShaderResourceView> srv;
   if (FAILED(device.CreateShaderResourceView(tex.get(), nullptr,
                                              srv.clear_and_assign()))) {
      log_and_terminate("Failed to create SRV for builtin!");
   }

   return srv;
}
}

Basic_builtin_textures::Basic_builtin_textures(ID3D11Device5& device) noexcept
{
   white = make_simple_texture(device, {0xff, 0xff, 0xff, 0xff});
   grey = make_simple_texture(device, {0x80, 0x80, 0x80, 0xff});
   normal = make_simple_texture(device, {0x80, 0x80, 0xff, 0xff});
}

void Basic_builtin_textures::add_to_database(Shader_resource_database& resources) noexcept
{
   resources.insert(white, "_SP_BUILTIN_white"sv);
   resources.insert(grey, "_SP_BUILTIN_grey"sv);

   resources.insert(white, "_SP_BUILTIN_null_ao"sv);
   resources.insert(white, "_SP_BUILTIN_null_depth"sv);
   resources.insert(grey, "_SP_BUILTIN_null_detailmap"sv);
   resources.insert(normal, "_SP_BUILTIN_null_normalmap"sv);
}

}
