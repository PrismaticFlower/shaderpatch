
#include "patch_texture_io.hpp"
#include "com_ptr.hpp"
#include "compose_exception.hpp"
#include "ucfb_reader.hpp"
#include "ucfb_writer.hpp"
#include "utility.hpp"
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

#include <comdef.h>
#include <d3d11_1.h>
#include <d3d9.h>

namespace sp {

using namespace std::literals;

enum class Texture_version : std::uint32_t { _1_0_0, _2_0_0, current = _2_0_0 };

inline namespace v_2_0_0 {
auto load_patch_texture_impl(ucfb::Reader reader, ID3D11Device1& device)
   -> std::pair<Com_ptr<ID3D11ShaderResourceView>, std::string>;
}

namespace v_1_0_0 {
auto load_patch_texture_impl(ucfb::Reader reader, ID3D11Device1& device)
   -> std::pair<Com_ptr<ID3D11ShaderResourceView>, std::string>;
}

auto load_patch_texture(ucfb::Reader reader, ID3D11Device1& device)
   -> std::pair<Com_ptr<ID3D11ShaderResourceView>, std::string>
{
   const auto name = reader.read_child_strict<"NAME"_mn>().read_string();

   const auto version =
      reader.read_child_strict<"VER_"_mn>().read<Texture_version>();

   reader.reset_head();

   switch (version) {
   case Texture_version::_1_0_0:
      return v_1_0_0::load_patch_texture_impl(reader, device);
   case Texture_version::current:
      return load_patch_texture_impl(reader, device);
   default:
      throw std::runtime_error{"texture has unknown version"};
   }
}

void write_patch_texture(const std::filesystem::path& save_path,
                         const Texture_info& texture_info,
                         const std::vector<Texture_data>& texture_data)
{
   using namespace std::literals;
   namespace fs = std::filesystem;

   std::ostringstream ostream;

   // write sptx chunk
   {
      ucfb::Writer writer{ostream, "sptx"_mn};

      writer.emplace_child("NAME"_mn).write(save_path.stem().string());
      writer.emplace_child("VER_"_mn).write(Texture_version::current);
      writer.emplace_child("INFO"_mn).write(texture_info);

      // write texture data
      {
         auto data_writer = writer.emplace_child("DATA"_mn);

         for (const auto& data : texture_data) {
            auto sub_writer = data_writer.emplace_child("SUB_"_mn);

            sub_writer.write(data.pitch, data.slice_pitch);
            sub_writer.write(gsl::narrow_cast<std::uint32_t>(data.data.size()));
            sub_writer.write_unaligned(data.data); // Much data, such wow.
         }
      }
   }

   const auto sptx_data = ostream.str();
   const auto sptx_span =
      gsl::span<const std::byte>(reinterpret_cast<const std::byte*>(sptx_data.data()),
                                 sptx_data.size());

   save_volume_resource(save_path.string(), save_path.stem().string(),
                        Volume_resource_type::texture, sptx_span);
}

namespace v_2_0_0 {

auto create_srv(ID3D11Device1& device, ID3D11Resource& resource,
                const std::string_view name) -> Com_ptr<ID3D11ShaderResourceView>
{
   Com_ptr<ID3D11ShaderResourceView> srv;

   if (const auto result = device.CreateShaderResourceView(&resource, nullptr,
                                                           srv.clear_and_assign());
       FAILED(result)) {
      throw compose_exception<std::runtime_error>("failed to create SRV for texture "sv,
                                                  std::quoted(name), '.');
   }

   return srv;
}

auto create_texture1d(ID3D11Device1& device, const Texture_info& info,
                      const std::vector<D3D11_SUBRESOURCE_DATA>& init_data,
                      const std::string_view name) -> Com_ptr<ID3D11ShaderResourceView>
{
   Expects(info.type == Texture_type::texture1d ||
           info.type == Texture_type::texture1darray);
   Expects(info.height == 1 && info.depth == 1);

   Com_ptr<ID3D11Texture1D> texture;

   const auto d3d11_desc = CD3D11_TEXTURE1D_DESC{info.format,
                                                 info.width,
                                                 info.array_size,
                                                 info.mip_count,
                                                 D3D11_BIND_SHADER_RESOURCE,
                                                 D3D11_USAGE_IMMUTABLE};

   if (const auto result = device.CreateTexture1D(&d3d11_desc, init_data.data(),
                                                  texture.clear_and_assign());
       FAILED(result)) {
      throw compose_exception<std::runtime_error>("failed to create texture "sv,
                                                  std::quoted(name), '.');
   }

   return create_srv(device, *texture, name);
}

auto create_texture2d(ID3D11Device1& device, const Texture_info& info,
                      const std::vector<D3D11_SUBRESOURCE_DATA>& init_data,
                      const std::string_view name) -> Com_ptr<ID3D11ShaderResourceView>
{
   Expects(info.type == Texture_type::texture2d ||
           info.type == Texture_type::texture2darray);
   Expects(info.depth == 1);

   Com_ptr<ID3D11Texture2D> texture;

   const auto d3d11_desc =
      CD3D11_TEXTURE2D_DESC{info.format,          info.width,
                            info.height,          info.array_size,
                            info.mip_count,       D3D11_BIND_SHADER_RESOURCE,
                            D3D11_USAGE_IMMUTABLE};

   if (const auto result = device.CreateTexture2D(&d3d11_desc, init_data.data(),
                                                  texture.clear_and_assign());
       FAILED(result)) {
      throw compose_exception<std::runtime_error>("failed to create texture "sv,
                                                  std::quoted(name), '.');
   }

   return create_srv(device, *texture, name);
}

auto create_texture3d(ID3D11Device1& device, const Texture_info& info,
                      const std::vector<D3D11_SUBRESOURCE_DATA>& init_data,
                      const std::string_view name) -> Com_ptr<ID3D11ShaderResourceView>
{
   Expects(info.type == Texture_type::texture3d);
   Expects(info.array_size == 1);

   Com_ptr<ID3D11Texture3D> texture;

   const auto d3d11_desc =
      CD3D11_TEXTURE3D_DESC{info.format,          info.width,
                            info.height,          info.depth,
                            info.mip_count,       D3D11_BIND_SHADER_RESOURCE,
                            D3D11_USAGE_IMMUTABLE};

   if (const auto result = device.CreateTexture3D(&d3d11_desc, init_data.data(),
                                                  texture.clear_and_assign());
       FAILED(result)) {
      throw compose_exception<std::runtime_error>("failed to create texture "sv,
                                                  std::quoted(name), '.');
   }

   return create_srv(device, *texture, name);
}

auto create_texturecube(ID3D11Device1& device, const Texture_info& info,
                        const std::vector<D3D11_SUBRESOURCE_DATA>& init_data,
                        const std::string_view name) -> Com_ptr<ID3D11ShaderResourceView>
{
   Expects(info.type == Texture_type::texturecube ||
           info.type == Texture_type::texturecubearray);
   Expects(info.depth == 1);

   Com_ptr<ID3D11Texture2D> texture;

   const auto d3d11_desc = CD3D11_TEXTURE2D_DESC{info.format,
                                                 info.width,
                                                 info.height,
                                                 info.array_size,
                                                 info.mip_count,
                                                 D3D11_BIND_SHADER_RESOURCE,
                                                 D3D11_USAGE_IMMUTABLE,
                                                 1,
                                                 0,
                                                 D3D10_RESOURCE_MISC_TEXTURECUBE};

   if (const auto result = device.CreateTexture2D(&d3d11_desc, init_data.data(),
                                                  texture.clear_and_assign());
       FAILED(result)) {
      throw compose_exception<std::runtime_error>("failed to create texture "sv,
                                                  std::quoted(name), '.');
   }

   return create_srv(device, *texture, name);
}

auto load_patch_texture_impl(ucfb::Reader reader, ID3D11Device1& device)
   -> std::pair<Com_ptr<ID3D11ShaderResourceView>, std::string>
{
   using namespace std::literals;

   const auto name = reader.read_child_strict<"NAME"_mn>().read_string();

   const auto version =
      reader.read_child_strict<"VER_"_mn>().read<Texture_version>();

   Ensures(version == Texture_version::_2_0_0);

   const auto info = reader.read_child_strict<"INFO"_mn>().read<Texture_info>();

   const auto sub_res_count = info.array_size * info.mip_count;

   std::vector<D3D11_SUBRESOURCE_DATA> init_data;
   init_data.reserve(sub_res_count);

   auto data = reader.read_child_strict<"DATA"_mn>();

   for (auto i = 0; i < sub_res_count; ++i) {
      auto sub = data.read_child_strict<"SUB_"_mn>();

      const auto [pitch, slice_pitch, sub_data_size] =
         sub.read_multi<UINT, UINT, std::uint32_t>();

      const auto sub_data = sub.read_array_unaligned<std::byte>(sub_data_size);

      D3D11_SUBRESOURCE_DATA subres_data{};

      init_data.push_back({sub_data.data(), pitch, slice_pitch});
   }

   Com_ptr<ID3D11ShaderResourceView> srv;

   switch (info.type) {
   case Texture_type::texture1d:
   case Texture_type::texture1darray:
      return {create_texture1d(device, info, init_data, name), std::string{name}};
   case Texture_type::texture2d:
   case Texture_type::texture2darray:
      return {create_texture2d(device, info, init_data, name), std::string{name}};
   case Texture_type::texture3d:
      return {create_texture3d(device, info, init_data, name), std::string{name}};
   case Texture_type::texturecube:
   case Texture_type::texturecubearray:
      return {create_texturecube(device, info, init_data, name), std::string{name}};
   default:
      std::terminate();
   }
}
}

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

auto map_format(const D3DFORMAT format, const BOOL srgb) -> DXGI_FORMAT
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

auto load_patch_texture_impl(ucfb::Reader reader, ID3D11Device1& device)
   -> std::pair<Com_ptr<ID3D11ShaderResourceView>, std::string>
{
   using namespace std::literals;

   const auto name = reader.read_child_strict<"NAME"_mn>().read_string();

   const auto version =
      reader.read_child_strict<"VER_"_mn>().read<Texture_version>();

   Ensures(version == Texture_version::_1_0_0);

   const auto info = reader.read_child_strict<"INFO"_mn>().read<Texture_info>();

   if (info._reserved != 0) {
      throw compose_exception<std::runtime_error>("Reeserved value in texture "sv,
                                                  std::quoted(name),
                                                  " is non-zero."sv);
   }

   const auto sampler_info =
      reader.read_child_strict<"SAMP"_mn>().read<Sampler_info>();

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

      std::size_t row_pitch;
      std::size_t slice_pitch;

      if (const auto result =
             DirectX::ComputePitch(dxgi_format, info.width >> i,
                                   info.height >> i, row_pitch, slice_pitch);
          FAILED(result)) {
         throw compose_exception<std::runtime_error>("Failed to compute pitch for texture "sv,
                                                     std::quoted(name), '.');
      }

      D3D11_SUBRESOURCE_DATA subres_data{nullptr, gsl::narrow_cast<UINT>(row_pitch),
                                         gsl::narrow_cast<UINT>(slice_pitch)};

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

   Com_ptr<ID3D11ShaderResourceView> srv;
   {
      if (const auto result =
             device.CreateShaderResourceView(texture.get(), nullptr,
                                             srv.clear_and_assign());
          FAILED(result)) {
         throw compose_exception<std::runtime_error>(
            "Failed to create game texture SRV! reason: ",
            _com_error{result}.ErrorMessage());
      }
   }

   return {srv, std::string{name}};
}

}
}
