
#include "upload_texture.hpp"
#include "utility.hpp"

#include <memory>

#include <DirectXTex.h>
#include <glm/glm.hpp>

namespace sp::d3d9 {

namespace {

auto calc_size(const DXGI_FORMAT format, const UINT width, const UINT height,
               const UINT depth, const UINT mip_levels, const UINT array_size)
   -> std::size_t
{
   std::size_t needed_size = sizeof(core::Mapped_texture) * (mip_levels * array_size);

   needed_size = next_multiple_of<Upload_texture::alignment>(needed_size);

   for (auto index = 0u; index < array_size; ++index) {
      for (auto mip = 0u; mip < mip_levels; ++mip) {
         std::size_t row_pitch{};
         std::size_t slice_pitch{};

         if (const auto result =
                DirectX::ComputePitch(format, glm::max(width >> mip, std::size_t{1}),
                                      glm::max(height >> mip, std::size_t{1}),
                                      row_pitch, slice_pitch);
             FAILED(result))
            std::terminate();

         const auto mip_depth = glm::max(depth >> mip, std::size_t{1});

         needed_size = next_multiple_of<Upload_texture::alignment>(
            needed_size + slice_pitch * mip_depth);
      }
   }

   return needed_size;
}
}

Upload_texture::Upload_texture(Upload_scratch_buffer& scratch_buffer,
                               const DXGI_FORMAT format, const UINT width,
                               const UINT height, const UINT depth,
                               const UINT mip_levels, const UINT array_size) noexcept
   : _scratch_buffer{scratch_buffer}, _mip_levels{mip_levels}
{
   Expects(mip_levels > 0 && array_size >= 1);

   const auto size = calc_size(format, width, height, depth, mip_levels, array_size);
   auto* const data = scratch_buffer.lock(size);

   _surfaces = {reinterpret_cast<core::Mapped_texture*>(data), array_size * mip_levels};

   std::uninitialized_default_construct(_surfaces.begin(), _surfaces.end());

   auto data_offset = next_multiple_of<Upload_texture::alignment>(
      sizeof(core::Mapped_texture) * (array_size * mip_levels));

   for (auto index = 0u; index < array_size; ++index) {
      for (auto mip = 0u; mip < mip_levels; ++mip) {
         auto& surface = _surfaces[index * mip_levels + mip];

         surface.data = data + data_offset;

         if (const auto result =
                DirectX::ComputePitch(format, glm::max(width >> mip, std::size_t{1}),
                                      glm::max(height >> mip, std::size_t{1}),
                                      surface.row_pitch, surface.depth_pitch);
             FAILED(result))
            std::terminate();

         const auto mip_depth = glm::max(depth >> mip, std::size_t{1});

         data_offset = next_multiple_of<Upload_texture::alignment>(
            data_offset + surface.depth_pitch * mip_depth);
      }
   }
}

Upload_texture::~Upload_texture()
{
   static_assert(
      std::is_trivially_destructible_v<core::Mapped_texture>,
      "An array of core::Mapped_texture was explicitly constructed into the "
      "scratch buffer but was not explicitly destroyed. (Trivial "
      "destructibility was expected.)");

   _scratch_buffer.unlock();
}

auto Upload_texture::subresource(const UINT mip, const UINT index) noexcept
   -> core::Mapped_texture
{
   return _surfaces[index * _mip_levels + mip];
}

auto Upload_texture::subresources() noexcept -> std::span<const core::Mapped_texture>
{
   return _surfaces;
}

}
