#pragma once

#include "com_ptr.hpp"

#include <span>
#include <string_view>

#include <d3d11_4.h>

namespace sp::material {

enum class Resource_type {
   buffer,
   texture1d,
   texture2d,
   texture3d,
   texture_cube
};

struct Resource_info {
   Resource_type type;
   std::uint32_t width;
   std::uint32_t buffer_length;
   std::uint32_t height;
   std::uint32_t depth;
   std::uint32_t array_size;
   std::uint32_t mip_levels;
};

class Resource_info_view {
public:
   Resource_info_view(std::span<const Com_ptr<ID3D11ShaderResourceView>> shader_resources) noexcept;

   Resource_info_view(const Resource_info_view&) = default;
   Resource_info_view& operator=(const Resource_info_view&) = default;

   Resource_info_view(Resource_info_view&&) = delete;
   Resource_info_view& operator=(Resource_info_view&&) = delete;

   ~Resource_info_view() = default;

   auto get(const std::size_t index) const noexcept -> Resource_info;

private:
   const std::span<const Com_ptr<ID3D11ShaderResourceView>> _shader_resources;
};

struct Resource_info_views {
   Resource_info_view vs;
   Resource_info_view ps;
};

}
