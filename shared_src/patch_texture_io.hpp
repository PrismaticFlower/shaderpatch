#pragma once

#include "com_ptr.hpp"
#include "compose_exception.hpp"
#include "ucfb_reader.hpp"
#include "ucfb_writer.hpp"
#include "volume_resource.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <execution>
#include <filesystem>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <DirectXTex.h>
#include <gsl/gsl>

#include <d3d11_1.h>
#include <d3d9.h>

namespace sp {

enum class Texture_version : std::uint32_t { _1_0_0, current = _1_0_0 };

namespace patch_textures {

inline namespace v_1_0_0 {
inline auto load_patch_texture(ucfb::Reader reader, ID3D11Device1& device)
   -> std::pair<Com_ptr<ID3D11Texture2D>, std::string>;
}

}

inline auto load_patch_texture(ucfb::Reader reader, ID3D11Device1& device)
   -> std::pair<Com_ptr<ID3D11Texture2D>, std::string>
{
   const auto name = reader.read_child_strict<"NAME"_mn>().read_string();

   const auto version =
      reader.read_child_strict<"VER_"_mn>().read_trivial<Texture_version>();

   reader.reset_head();

   switch (version) {
   case Texture_version::current:
      return patch_textures::load_patch_texture(reader, device);
   default:
      throw std::runtime_error{"texture has unknown version"};
   }
}

// inline void write_patch_texture(const std::filesystem::path& save_path,
//                                 const Texture_info& texture_info,
//                                 const Sampler_info& sampler_info,
//                                 const std::vector<std::vector<std::byte>>& mip_levels)
// {
//    using namespace std::literals;
//    namespace fs = std::filesystem;
//
//    std::ostringstream ostream;
//
//    // write sptx chunk
//    {
//       ucfb::Writer writer{ostream, "sptx"_mn};
//
//       writer.emplace_child("NAME"_mn).write(save_path.stem().string());
//       writer.emplace_child("VER_"_mn).write(Texture_version::_1);
//       writer.emplace_child("INFO"_mn).write(texture_info);
//       writer.emplace_child("SAMP"_mn).write(sampler_info);
//
//       // write mip levels
//       {
//          auto mips_writer = writer.emplace_child("MIPS"_mn);
//
//          for (const auto& level : mip_levels) {
//             mips_writer.emplace_child("MIP_"_mn).write_unaligned(gsl::make_span(level));
//          }
//       }
//    }
//
//    const auto sptx_data = ostream.str();
//    const auto sptx_span =
//       gsl::span<const std::byte>(reinterpret_cast<const std::byte*>(sptx_data.data()),
//                                  sptx_data.size());
//
//    save_volume_resource(save_path.string(), save_path.stem().string(),
//                         Volume_resource_type::texture, sptx_span);
// }

namespace patch_textures {

namespace v_1_0_0 {

struct Texture_info {
   std::uint32_t _reserved{0};
   std::int32_t width;
   std::int32_t height;
   std::int32_t mip_count;
   D3DFORMAT format;
};

struct Sampler_info {
   D3DTEXTUREADDRESS address_mode_u;
   D3DTEXTUREADDRESS address_mode_v;
   D3DTEXTUREADDRESS address_mode_w;
   D3DTEXTUREFILTERTYPE mag_filter;
   D3DTEXTUREFILTERTYPE min_filter;
   D3DTEXTUREFILTERTYPE mip_filter;
   float mip_lod_bias;
   BOOL srgb;
};

inline auto map_format(const D3DFORMAT format, const BOOL srgb) -> DXGI_FORMAT
{
   constexpr auto ati1 = static_cast<D3DFORMAT>(MAKEFOURCC('A', 'T', 'I', '1'));
   constexpr auto ati2 = static_cast<D3DFORMAT>(MAKEFOURCC('A', 'T', 'I', '2'));

   switch (static_cast<int>(format)) {
   case D3DFMT_A8R8G8B8:
      return srgb ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB : DXGI_FORMAT_B8G8R8A8_UNORM;
   case D3DFMT_X8R8G8B8:
      return srgb ? DXGI_FORMAT_B8G8R8X8_UNORM_SRGB : DXGI_FORMAT_B8G8R8X8_UNORM;
   case D3DFMT_A8B8G8R8:
      return srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
   case D3DFMT_X8B8G8R8:
      return srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
   case D3DFMT_A16B16G16R16F:
      return DXGI_FORMAT_R8G8B8A8_UNORM;
   case D3DFMT_DXT1:
      return srgb ? DXGI_FORMAT_BC1_UNORM_SRGB : DXGI_FORMAT_BC1_UNORM;
   case D3DFMT_DXT2:
      return srgb ? DXGI_FORMAT_BC2_UNORM_SRGB : DXGI_FORMAT_BC2_UNORM;
   case D3DFMT_DXT3:
      return srgb ? DXGI_FORMAT_BC2_UNORM_SRGB : DXGI_FORMAT_BC2_UNORM;
   case D3DFMT_DXT4:
      return srgb ? DXGI_FORMAT_BC3_UNORM_SRGB : DXGI_FORMAT_BC3_UNORM;
   case D3DFMT_DXT5:
      return srgb ? DXGI_FORMAT_BC3_UNORM_SRGB : DXGI_FORMAT_BC3_UNORM;
   case ati1:
      return DXGI_FORMAT_BC4_UNORM;
   case ati2:
      return DXGI_FORMAT_BC5_UNORM;
   }

   throw std::runtime_error{"Texture specifies unsupported format!"};
}

inline auto load_patch_texture(ucfb::Reader reader, ID3D11Device1& device)
   -> std::pair<Com_ptr<ID3D11Texture2D>, std::string>
{
   using namespace std::literals;

   const auto name = reader.read_child_strict<"NAME"_mn>().read_string();

   const auto version =
      reader.read_child_strict<"VER_"_mn>().read_trivial<Texture_version>();

   Ensures(version == Texture_version::_1_0_0);

   const auto info =
      reader.read_child_strict<"INFO"_mn>().read_trivial<Texture_info>();

   if (info._reserved != 0) {
      throw compose_exception<std::runtime_error>("Reeserved value in texture "sv,
                                                  std::quoted(name),
                                                  " is non-zero."sv);
   }

   const auto sampler_info =
      reader.read_child_strict<"SAMP"_mn>().read_trivial<Sampler_info>();

   using Block_pair = std::array<std::uint64_t, 2>;
   std::vector<std::unique_ptr<Block_pair[]>> init_patchup_buffers;
   init_patchup_buffers.reserve(info.mip_count);
   std::vector<D3D11_SUBRESOURCE_DATA> init_data;
   init_data.reserve(info.mip_count);

   const auto dxgi_format = map_format(info.format, sampler_info.srgb);
   const auto patchup_bc5_layout = dxgi_format == DXGI_FORMAT_BC5_UNORM;

   auto mips = reader.read_child_strict<"MIPS"_mn>();

   for (auto i = 0; i < info.mip_count; ++i) {
      auto level = mips.read_child_strict<"MIP_"_mn>();
      auto data = level.read_array_unaligned<std::byte>(level.size());

      D3D11_SUBRESOURCE_DATA subres_data{};

      if (const auto result =
             DirectX::ComputePitch(dxgi_format, info.width >> i, info.height >> i,
                                   subres_data.SysMemPitch, subres_data.SysMemSlicePitch);
          FAILED(result)) {
         throw compose_exception<std::runtime_error>("Failed to compute pitch for texture "sv,
                                                     std::quoted(name), '.');
      }

      // Convert ATI2 block layout to BC5 block layout.
      if (patchup_bc5_layout) {
         const auto block_pair_count = data.size() / sizeof(Block_pair);

         auto& patchup_buffer = init_patchup_buffers.emplace_back(
            std::make_unique<Block_pair[]>(block_pair_count));

         std::memcpy(patchup_buffer.get(), data.data(), data.size());

         std::for_each_n(
            std::execution::par_unseq, patchup_buffer.get(), block_pair_count,
            [](Block_pair & block_pair) noexcept {
               std::swap(block_pair[0], block_pair[1]);
            });

         subres_data.pSysMem = patchup_buffer.get();
      }
      else {
         subres_data.pSysMem = data.data();
      }

      init_data.emplace_back(subres_data);
   }

   Com_ptr<ID3D11Texture2D> texture;

   const auto d3d11_desc = CD3D11_TEXTURE2D_DESC{dxgi_format,
                                                 static_cast<UINT>(info.width),
                                                 static_cast<UINT>(info.height),
                                                 1,
                                                 static_cast<UINT>(info.mip_count),
                                                 D3D11_BIND_SHADER_RESOURCE,
                                                 D3D11_USAGE_IMMUTABLE};

   if (const auto result = device.CreateTexture2D(&d3d11_desc, init_data.data(),
                                                  texture.clear_and_assign());
       FAILED(result)) {
      throw compose_exception<std::runtime_error>("failed to create texture "sv,
                                                  std::quoted(name), '.');
   }

   return {texture, std::string{name}};
}

}
}
}
