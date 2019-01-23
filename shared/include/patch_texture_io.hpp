#pragma once

#include "com_ptr.hpp"
#include "ucfb_reader.hpp"

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include <gsl/gsl>

#include <d3d11_1.h>

namespace sp {

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
   gsl::span<const std::byte> data;
};

auto load_patch_texture(ucfb::Reader reader, ID3D11Device1& device)
   -> std::pair<Com_ptr<ID3D11ShaderResourceView>, std::string>;

void write_patch_texture(const std::filesystem::path& save_path,
                         const Texture_info& texture_info,
                         const std::vector<Texture_data>& texture_data);

}
