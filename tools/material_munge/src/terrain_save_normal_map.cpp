
#include "terrain_save_normal_map.hpp"
#include "image_span.hpp"
#include "patch_texture_io.hpp"
#include "terrain_constants.hpp"
#include "throw_if_failed.hpp"

#include <execution>

#include <DirectXTex.h>

using namespace std::literals;

namespace sp {

namespace {

auto create_terrain_normal_map(const Terrain_map& terrain) -> std::vector<glm::vec3>
{
   Expects(terrain.length >= 2);

   std::vector<glm::vec3> normals;
   normals.resize(terrain.length * terrain.length);

   for (auto y = 0; y < (terrain.length - 1); ++y) {
      for (auto x = 0; x < (terrain.length - 1); ++x) {
         const auto i0 = x + (y * terrain.length);
         const auto i1 = x + ((y + 1) * terrain.length);
         const auto i2 = (x + 1) + (y * terrain.length);
         const auto i3 = (x + 1) + ((y + 1) * terrain.length);

         const auto v0 = terrain.position[i0];
         const auto v1 = terrain.position[i1];
         const auto v2 = terrain.position[i2];
         const auto v3 = terrain.position[i3];

         // tri 0
         {
            const glm::vec3 e1 = terrain.position[i2] - terrain.position[i0];
            const glm::vec3 e2 = terrain.position[i3] - terrain.position[i0];
            const glm::vec3 normal = glm::cross(e1, e2);

            normals[i0] += normal;
            normals[i2] += normal;
            normals[i3] += normal;
         }

         // tri 1
         {
            const glm::vec3 e1 = terrain.position[i3] - terrain.position[i0];
            const glm::vec3 e2 = terrain.position[i1] - terrain.position[i0];
            const glm::vec3 normal = glm::cross(e1, e2);

            normals[i0] += normal;
            normals[i3] += normal;
            normals[i1] += normal;
         }
      }
   }

   return normals;
}

auto pack_terrain_normal_map(const Terrain_map& terrain,
                             std::vector<glm::vec3> normals) -> DirectX::ScratchImage
{
   const DirectX::ScratchImage mipped_normals = [&] {
      DirectX::ScratchImage output;

      const DirectX::Image input_normals{terrain.length,
                                         terrain.length,
                                         DXGI_FORMAT_R32G32B32_FLOAT,
                                         terrain.length * sizeof(glm::vec3),
                                         terrain.length *
                                            (terrain.length * sizeof(glm::vec3)),
                                         reinterpret_cast<std::uint8_t*>(normals.data())};

      throw_if_failed(DirectX::GenerateMipMaps(input_normals,
                                               DirectX::TEX_FILTER_DEFAULT |
                                                  DirectX::TEX_FILTER_FORCE_NON_WIC,
                                               0, output));

      return output;
   }();

   DirectX::ScratchImage packed_image;
   packed_image.Initialize2D(DXGI_FORMAT_R10G10B10A2_UNORM, terrain.length,
                             terrain.length, 1,
                             mipped_normals.GetMetadata().mipLevels);

   for (auto i = 0; i < mipped_normals.GetMetadata().mipLevels; ++i) {
      const Image_span in_span{*mipped_normals.GetImage(i, 0, 0)};
      Image_span out_span{*packed_image.GetImage(i, 0, 0)};

      for_each(std::execution::par_unseq, in_span, [&](const glm::ivec2 index) {
         out_span.store(index,
                        glm::vec4{glm::normalize(glm::vec3{in_span.load(index)}) * 0.5f + 0.5f,
                                  1.0f});
      });
   }

   return packed_image;
}
}

void terrain_save_normal_map(const Terrain_map& terrain, const std::string_view suffix,
                             const std::filesystem::path& output_munge_files_dir)
{
   const auto normal_map =
      pack_terrain_normal_map(terrain, create_terrain_normal_map(terrain));

   auto meta = normal_map.GetMetadata();

   Texture_info info;
   info.type = Texture_type::texture2d;
   info.width = static_cast<std::uint32_t>(meta.width);
   info.height = static_cast<std::uint32_t>(meta.height);
   info.depth = 1;
   info.array_size = 1;
   info.mip_count = static_cast<std::uint32_t>(meta.mipLevels);
   info.format = meta.format;

   std::vector<Texture_data> texture_data;
   texture_data.reserve(meta.mipLevels);

   for (auto mip = 0; mip < meta.mipLevels; ++mip) {
      auto* const sub_image = normal_map.GetImage(mip, 0, 0);

      Texture_data data;
      data.pitch = gsl::narrow_cast<UINT>(sub_image->rowPitch);
      data.slice_pitch = gsl::narrow_cast<UINT>(sub_image->slicePitch);
      data.data = gsl::make_span(reinterpret_cast<std::byte*>(sub_image->pixels),
                                 sub_image->slicePitch);

      texture_data.emplace_back(data);
   }

   const auto outputpath =
      output_munge_files_dir /
      ((std::string{terrain_normal_map_texture_name} += suffix) += ".sptex"sv);

   write_patch_texture(outputpath, info, texture_data,
                       Texture_file_type::volume_resource);
}
}
