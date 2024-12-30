
#include "resource_info_view.hpp"
#include "../logger.hpp"

namespace sp::material {

Resource_info_view::Resource_info_view(
   std::span<const Com_ptr<ID3D11ShaderResourceView>> shader_resources) noexcept
   : _shader_resources{shader_resources}
{
}

auto Resource_info_view::get(const std::size_t index) const noexcept -> Resource_info
{
   if (index >= _shader_resources.size()) {
      log_and_terminate_fmt("No such resource at index '{}'", index);
   }

   if (!_shader_resources[index]) return {};

   Com_ptr<ID3D11Resource> resource;

   _shader_resources[index]->GetResource(resource.clear_and_assign());

   D3D11_RESOURCE_DIMENSION resource_dimension{};

   resource->GetType(&resource_dimension);

   switch (resource_dimension) {
   case D3D11_RESOURCE_DIMENSION_BUFFER: {
      Com_ptr<ID3D11Buffer> buffer;

      resource->QueryInterface(buffer.clear_and_assign());

      D3D11_BUFFER_DESC desc;

      buffer->GetDesc(&desc);

      return {.type = Resource_type::buffer, .buffer_length = desc.ByteWidth};
   }
   case D3D11_RESOURCE_DIMENSION_TEXTURE1D: {
      Com_ptr<ID3D11Texture1D> texture;

      resource->QueryInterface(texture.clear_and_assign());

      D3D11_TEXTURE1D_DESC desc;

      texture->GetDesc(&desc);

      return {.type = Resource_type::texture1d,
              .width = desc.Width,
              .array_size = desc.ArraySize,
              .mip_levels = desc.MipLevels};
   }
   case D3D11_RESOURCE_DIMENSION_TEXTURE2D: {
      Com_ptr<ID3D11Texture2D> texture;

      resource->QueryInterface(texture.clear_and_assign());

      D3D11_TEXTURE2D_DESC desc;

      texture->GetDesc(&desc);

      if (desc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE) {
         return {.type = Resource_type::texture_cube,
                 .width = desc.Width,
                 .height = desc.Height,
                 .array_size = desc.ArraySize / 6u,
                 .mip_levels = desc.MipLevels};
      }
      else {
         return {.type = Resource_type::texture2d,
                 .width = desc.Width,
                 .height = desc.Height,
                 .array_size = desc.ArraySize,
                 .mip_levels = desc.MipLevels};
      }
   }

   case D3D11_RESOURCE_DIMENSION_TEXTURE3D: {
      Com_ptr<ID3D11Texture3D> texture;

      resource->QueryInterface(texture.clear_and_assign());

      D3D11_TEXTURE3D_DESC desc;

      texture->GetDesc(&desc);

      return {.type = Resource_type::texture3d,
              .width = desc.Width,
              .height = desc.Height,
              .depth = desc.Depth,
              .mip_levels = desc.MipLevels};
   }

   default:
      return {};
   }
}
}