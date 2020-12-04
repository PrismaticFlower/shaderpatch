
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

enum class Texture_version : std::uint32_t { v_1, current = v_1 };

namespace {
inline namespace v_1 {
auto load_patch_texture_impl(ucfb::Reader_strict<"sptx"_mn> reader, ID3D11Device1& device)
   -> std::pair<Com_ptr<ID3D11ShaderResourceView>, std::string>;

void load_patch_texture_impl(
   ucfb::Reader_strict<"sptx"_mn> reader,
   std::function<void(const Texture_info info)> info_callback,
   std::function<void(const std::uint32_t item, const std::uint32_t mip, const Texture_data data)> data_callback);
}

void write_sptx(ucfb::File_writer& writer, const std::string_view name,
                const Texture_info& texture_info,
                const std::vector<Texture_data>& texture_data);

}

auto load_patch_texture(ucfb::Reader_strict<"sptx"_mn> reader, ID3D11Device1& device)
   -> std::pair<Com_ptr<ID3D11ShaderResourceView>, std::string>
{
   const auto version =
      reader.read_child_strict<"VER_"_mn>().read<Texture_version>();

   reader.reset_head();

   switch (version) {
   case Texture_version::current:
      return load_patch_texture_impl(reader, device);
   default:
      throw std::runtime_error{"texture has unknown version"};
   }
}

void load_patch_texture(
   ucfb::Reader_strict<"sptx"_mn> reader,
   std::function<void(const Texture_info info)> info_callback,
   std::function<void(const std::uint32_t item, const std::uint32_t mip, const Texture_data data)> data_callback)
{
   const auto version =
      reader.read_child_strict<"VER_"_mn>().read<Texture_version>();

   reader.reset_head();

   switch (version) {
   case Texture_version::current:
      return load_patch_texture_impl(reader, std::move(info_callback),
                                     std::move(data_callback));
   default:
      throw std::runtime_error{"texture has unknown version"};
   }
}

void write_patch_texture(ucfb::File_writer& writer, const std::string_view name,
                         const Texture_info& texture_info,
                         const std::vector<Texture_data>& texture_data,
                         const Texture_file_type file_type)
{
   if (file_type == Texture_file_type::volume_resource) {
      std::ostringstream string_stream;

      {
         ucfb::File_writer sptx{"sptx"_mn, string_stream};

         write_sptx(sptx, name, texture_info, texture_data);
      }

      const auto sptx_data = string_stream.str();
      const auto sptx_span = std::as_bytes(std::span{sptx_data});

      write_volume_resource(writer, name, Volume_resource_type::texture, sptx_span);
   }
   else {
      auto sptx = writer.emplace_child("sptx"_mn);

      write_sptx(sptx, name, texture_info, texture_data);
   }
}

void write_patch_texture(const std::filesystem::path& save_path,
                         const Texture_info& texture_info,
                         const std::vector<Texture_data>& texture_data,
                         const Texture_file_type file_type)
{
   if (file_type == Texture_file_type::volume_resource) {
      std::ostringstream string_stream;

      {
         ucfb::File_writer writer{"sptx"_mn, string_stream};

         write_sptx(writer, save_path.stem().string(), texture_info, texture_data);
      }

      const auto sptx_data = string_stream.str();
      const auto sptx_span = std::as_bytes(std::span{sptx_data});

      save_volume_resource(save_path, save_path.stem().string(),
                           Volume_resource_type::texture, sptx_span);
   }
   else {
      auto output_file = ucfb::open_file_for_output(save_path);

      ucfb::File_writer writer{"ucfb"_mn, output_file};

      {
         auto sptx_writer = writer.emplace_child("sptx"_mn);

         write_sptx(sptx_writer, save_path.stem().string(), texture_info, texture_data);
      }
   }
}

namespace {
namespace v_1 {

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
                                                 0,
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

auto load_patch_texture_impl(ucfb::Reader_strict<"sptx"_mn> reader, ID3D11Device1& device)
   -> std::pair<Com_ptr<ID3D11ShaderResourceView>, std::string>
{
   using namespace std::literals;

   const auto version =
      reader.read_child_strict<"VER_"_mn>().read<Texture_version>();

   Ensures(version == Texture_version::v_1);

   const auto name = reader.read_child_strict<"NAME"_mn>().read_string();

   const auto info = reader.read_child_strict<"INFO"_mn>().read<Texture_info>();

   const auto sub_res_count = info.array_size * info.mip_count;

   std::vector<D3D11_SUBRESOURCE_DATA> init_data;
   init_data.reserve(sub_res_count);

   auto data = reader.read_child_strict<"DATA"_mn>();

   for (auto i = 0; i < sub_res_count; ++i) {
      auto sub = data.read_child_strict<"SUB_"_mn>();

      const auto [pitch, slice_pitch, sub_data_size, data_offset] =
         sub.read_multi<UINT, UINT, std::uint32_t, std::uint32_t>();

      sub.consume_unaligned(data_offset);

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

void load_patch_texture_impl(
   ucfb::Reader_strict<"sptx"_mn> reader,
   std::function<void(const Texture_info info)> info_callback,
   std::function<void(const std::uint32_t item, const std::uint32_t mip, const Texture_data data)> data_callback)
{
   using namespace std::literals;

   const auto version =
      reader.read_child_strict<"VER_"_mn>().read<Texture_version>();

   Ensures(version == Texture_version::v_1);

   const auto name = reader.read_child_strict<"NAME"_mn>().read_string();

   const auto info = reader.read_child_strict<"INFO"_mn>().read<Texture_info>();

   info_callback(info);

   const auto sub_res_count = info.array_size * info.mip_count;

   std::vector<D3D11_SUBRESOURCE_DATA> init_data;
   init_data.reserve(sub_res_count);

   auto data = reader.read_child_strict<"DATA"_mn>();

   for (auto item = 0; item < info.array_size; ++item) {
      for (auto mip = 0; mip < info.mip_count; ++mip) {
         auto sub = data.read_child_strict<"SUB_"_mn>();

         const auto [pitch, slice_pitch, sub_data_size, data_offset] =
            sub.read_multi<UINT, UINT, std::uint32_t, std::uint32_t>();

         sub.consume_unaligned(data_offset);

         const auto sub_data = sub.read_array_unaligned<std::byte>(sub_data_size);

         data_callback(item, mip, {pitch, slice_pitch, sub_data});
      }
   }
}
}

void write_sptx(ucfb::File_writer& writer, const std::string_view name,
                const Texture_info& texture_info,
                const std::vector<Texture_data>& texture_data)
{
   writer.emplace_child("VER_"_mn).write(Texture_version::current);
   writer.emplace_child("NAME"_mn).write(name);
   writer.emplace_child("INFO"_mn).write(texture_info);

   // write texture data
   {
      auto data_writer = writer.emplace_child("DATA"_mn);

      for (const auto& data : texture_data) {
         auto sub_writer = data_writer.emplace_child("SUB_"_mn);

         sub_writer.write(data.pitch, data.slice_pitch);
         sub_writer.write(gsl::narrow_cast<std::uint32_t>(data.data.size()));

         ucfb::write_at_alignment<16>(sub_writer, data.data);
      }
   }
}

}
}
