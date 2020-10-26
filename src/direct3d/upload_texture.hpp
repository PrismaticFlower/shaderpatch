#pragma once

#include "../core/shader_patch.hpp"
#include "upload_scratch_buffer.hpp"

#include <gsl/gsl>

#include <d3d11_1.h>

namespace sp::d3d9 {

class Upload_texture {
public:
   constexpr static std::size_t alignment = 256u;

   Upload_texture(Upload_scratch_buffer& scratch_buffer, const DXGI_FORMAT format,
                  const UINT width, const UINT height, const UINT depth,
                  const UINT mip_levels, const UINT array_size) noexcept;

   ~Upload_texture();

   Upload_texture(const Upload_texture&) = delete;
   Upload_texture& operator=(const Upload_texture&) = delete;

   Upload_texture(Upload_texture&&) = delete;
   Upload_texture& operator=(Upload_texture&&) = delete;

   auto subresource(const UINT mip, const UINT index) noexcept -> core::Mapped_texture;

   auto subresources() noexcept -> std::span<const core::Mapped_texture>;

private:
   std::span<core::Mapped_texture> _surfaces;

   Upload_scratch_buffer& _scratch_buffer;
   const UINT _mip_levels;
};

}
