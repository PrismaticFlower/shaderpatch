#pragma once

#include "com_ptr.hpp"
#include "ucfb_reader.hpp"
#include "ucfb_writer.hpp"

#include <filesystem>
#include <functional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <d3d11_1.h>

namespace sp {

enum class Texture_file_type { volume_resource, direct_texture };

enum class Texture_type : std::uint32_t {
   texture1d,
   texture1darray,
   texture2d,
   texture2darray,
   texture3d,
   texturecube,
   texturecubearray
};

struct Texture_info {
   Texture_type type;
   std::uint32_t width;
   std::uint32_t height;
   std::uint32_t depth;
   std::uint32_t array_size;
   std::uint32_t mip_count;
   DXGI_FORMAT format;
};

struct Texture_data {
   UINT pitch;
   UINT slice_pitch;
   std::span<const std::byte> data;
};

auto load_patch_texture(ucfb::Reader_strict<"sptx"_mn> reader, ID3D11Device1& device)
   -> std::pair<Com_ptr<ID3D11ShaderResourceView>, std::string>;

void load_patch_texture(
   ucfb::Reader_strict<"sptx"_mn> reader,
   std::function<void(const Texture_info info)> info_callback,
   std::function<void(const std::uint32_t item, const std::uint32_t mip, const Texture_data data)> data_callback);

void write_patch_texture(ucfb::File_writer& writer, const std::string_view name,
                         const Texture_info& texture_info,
                         const std::vector<Texture_data>& texture_data,
                         const Texture_file_type file_type);

void write_patch_texture(const std::filesystem::path& save_path,
                         const Texture_info& texture_info,
                         const std::vector<Texture_data>& texture_data,
                         const Texture_file_type file_type);

}
