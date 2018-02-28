
#include "texture_loader.hpp"

#include <cstring>

#include <DirectXTex.h>

namespace sp {
namespace {

bool dxgi_to_d3dfmt(DXGI_FORMAT dxgi_format, D3DFORMAT& d3d_format)
{
   switch (dxgi_format) {
   case DXGI_FORMAT_B8G8R8A8_UNORM:
      d3d_format = D3DFMT_A8R8G8B8;
      return true;
   case DXGI_FORMAT_B8G8R8X8_UNORM:
      d3d_format = D3DFMT_X8R8G8B8;
      return true;
   case DXGI_FORMAT_B5G6R5_UNORM:
      d3d_format = D3DFMT_R5G6B5;
      return true;
   case DXGI_FORMAT_B5G5R5A1_UNORM:
      d3d_format = D3DFMT_A1R5G5B5;
      return true;
   case DXGI_FORMAT_B4G4R4A4_UNORM:
      d3d_format = D3DFMT_A4R4G4B4;
      return true;
   case DXGI_FORMAT_A8_UNORM:
      d3d_format = D3DFMT_A8;
      return true;
   case DXGI_FORMAT_R8G8B8A8_UNORM:
   case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
      d3d_format = D3DFMT_A8B8G8R8;
      return true;
   case DXGI_FORMAT_R16G16_UNORM:
      d3d_format = D3DFMT_G16R16;
      return true;
   case DXGI_FORMAT_R16G16B16A16_UNORM:
      d3d_format = D3DFMT_A16B16G16R16;
      return true;
   case DXGI_FORMAT_R8_UNORM:
      d3d_format = D3DFMT_L8;
      return true;
   case DXGI_FORMAT_R8G8_SNORM:
      d3d_format = D3DFMT_V8U8;
      return true;
   case DXGI_FORMAT_R8G8B8A8_SNORM:
      d3d_format = D3DFMT_Q8W8V8U8;
      return true;
   case DXGI_FORMAT_R16G16_SNORM:
      d3d_format = D3DFMT_V16U16;
      return true;
   case DXGI_FORMAT_G8R8_G8B8_UNORM:
      d3d_format = D3DFMT_R8G8_B8G8;
      return true;
   case DXGI_FORMAT_R8G8_B8G8_UNORM:
      d3d_format = D3DFMT_G8R8_G8B8;
      return true;
   case DXGI_FORMAT_BC1_UNORM:
      d3d_format = D3DFMT_DXT1;
      return true;
   case DXGI_FORMAT_BC2_UNORM:
      d3d_format = D3DFMT_DXT3;
      return true;
   case DXGI_FORMAT_BC3_UNORM:
      d3d_format = D3DFMT_DXT5;
      return true;
   case DXGI_FORMAT_R16_UNORM:
      d3d_format = D3DFMT_L16;
      return true;
   case DXGI_FORMAT_R16G16B16A16_SNORM:
      d3d_format = D3DFMT_Q16W16V16U16;
      return true;
   case DXGI_FORMAT_R16_FLOAT:
      d3d_format = D3DFMT_R16F;
      return true;
   case DXGI_FORMAT_R16G16_FLOAT:
      d3d_format = D3DFMT_G16R16F;
      return true;
   case DXGI_FORMAT_R16G16B16A16_FLOAT:
      d3d_format = D3DFMT_A16B16G16R16F;
      return true;
   case DXGI_FORMAT_R32_FLOAT:
      d3d_format = D3DFMT_R32F;
      return true;
   case DXGI_FORMAT_R32G32_FLOAT:
      d3d_format = D3DFMT_G32R32F;
      return true;
   case DXGI_FORMAT_R32G32B32A32_FLOAT:
      d3d_format = D3DFMT_A32B32G32R32F;
      return true;
   default:
      return false;
   }
}
}

auto load_dds_from_file(IDirect3DDevice9& device, std::wstring file) noexcept
   -> Com_ptr<IDirect3DTexture9>
{
   DirectX::TexMetadata metadata;
   DirectX::ScratchImage scratch_image;

   auto result = DirectX::LoadFromDDSFile(file.data(), DirectX::DDS_FLAGS_NONE,
                                          &metadata, scratch_image);

   if (result != S_OK) return nullptr;

   D3DFORMAT format;

   if (!dxgi_to_d3dfmt(metadata.format, format)) return nullptr;

   Com_ptr<IDirect3DTexture9> texture;

   result = device.CreateTexture(metadata.width, metadata.height,
                                 metadata.mipLevels, 0, format, D3DPOOL_MANAGED,
                                 texture.clear_and_assign(), nullptr);

   if (result != S_OK) return nullptr;

   const auto bytes_per_pixel = DirectX::BitsPerPixel(metadata.format) / 8u;

   for (auto i = 0u; i < metadata.mipLevels; ++i) {
      D3DLOCKED_RECT rect{};

      texture->LockRect(i, &rect, nullptr, D3DLOCK_DISCARD);

      const auto& image = scratch_image.GetImage(i, 0, 0);

      std::memcpy(rect.pBits, image->pixels, image->slicePitch);

      texture->UnlockRect(i);
   }

   return texture;
}
}
