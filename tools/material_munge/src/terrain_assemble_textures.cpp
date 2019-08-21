
#include "terrain_assemble_textures.hpp"
#include "com_ptr.hpp"
#include "compose_exception.hpp"
#include "image_span.hpp"
#include "patch_texture_io.hpp"
#include "string_utilities.hpp"
#include "terrain_constants.hpp"
#include "throw_if_failed.hpp"
#include "utility.hpp"
#include "volume_resource.hpp"

#include <array>
#include <execution>
#include <functional>
#include <optional>
#include <unordered_map>

#include <DirectXTex.h>

#include <d3d11.h>

using namespace std::literals;

namespace sp {

namespace {

bool should_assemble_textures(
   const Terrain_materials_config& config, const std::filesystem::path& sptex_files_dir,
   const std::array<std::reference_wrapper<const std::filesystem::path>, 3> output_files) noexcept
{
   auto last_output_time = std::filesystem::file_time_type::clock::now();

   for (auto& file : output_files) {
      if (!std::filesystem::exists(file)) return true;

      last_output_time =
         safe_min(last_output_time, std::filesystem::last_write_time(file));
   }

   std::filesystem::file_time_type last_input_time{};

   const auto add_input = [&](const std::string& input) {
      if (input.front() == '$' || input.empty()) return false;

      const auto file = sptex_files_dir / (input + ".sptex"s);

      if (!std::filesystem::exists(file)) return true;

      last_input_time =
         safe_max(last_input_time, std::filesystem::last_write_time(file));

      return false;
   };

   for (auto& matl : config.materials) {
      if (add_input(matl.second.albedo_map)) return true;
      if (add_input(matl.second.normal_map)) return true;
      if (add_input(matl.second.metallic_roughness_map)) return true;
      if (add_input(matl.second.ao_map)) return true;
      if (add_input(matl.second.height_map)) return true;
      if (add_input(matl.second.gloss_map)) return true;
      if (add_input(matl.second.diffuse_map)) return true;
   }

   return last_output_time < last_input_time;
}

auto create_d3d11_device() -> Com_ptr<ID3D11Device>
{
   Com_ptr<ID3D11Device> device;

   if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
                                nullptr, 0, D3D11_SDK_VERSION,
                                device.clear_and_assign(), nullptr, nullptr))) {
      return nullptr;
   }

   return device;
}

auto load_sptex(const std::filesystem::path& path) -> DirectX::ScratchImage
{
   auto [vr_header, vr_data] = load_volume_resource(path);

   if (vr_header.type != Volume_resource_type::texture) {
      throw std::runtime_error{
         "Bad munged SP texture. Try cleaning your munge files."};
   }

   DirectX::ScratchImage image;
   Texture_info texture_info;

   load_patch_texture(
      ucfb::Reader_strict<"sptx"_mn>{gsl::make_span(vr_data)},
      [&](const Texture_info& info) {
         switch (info.type) {
         case Texture_type::texture1d:
         case Texture_type::texture1darray:
            throw std::runtime_error{
               "Can't use 1D texture as input for terrain texture!"};
         case Texture_type::texture2d:
         case Texture_type::texture2darray:
            texture_info = info;
            return;
         case Texture_type::texture3d:
            throw std::runtime_error{
               "Can't use 3D texture as input for terrain texture!"};
         case Texture_type::texturecube:
         case Texture_type::texturecubearray:
            throw std::runtime_error{
               "Can't use cube texture as input for terrain texture!"};
            return;
         default:
            std::terminate();
         }
      },
      [&](const std::int32_t item, const std::int32_t mip, const Texture_data data) {
         if (item != 0 || mip != 0) return;

         DirectX::Image src;

         src.width = texture_info.width;
         src.height = texture_info.height;
         src.format = texture_info.format;
         src.rowPitch = data.pitch;
         src.slicePitch = data.slice_pitch;
         src.pixels = const_cast<std::uint8_t*>(
            reinterpret_cast<const std::uint8_t*>(data.data.data())); // Yikes!

         throw_if_failed(image.InitializeFromImage(src));
      });

   if (const auto format = image.GetMetadata().format; DirectX::IsCompressed(format)) {
      DirectX::ScratchImage compressed;
      std::swap(image, compressed);

      throw_if_failed(
         DirectX::Decompress(compressed.GetImages(), compressed.GetImageCount(),
                             compressed.GetMetadata(), DXGI_FORMAT_UNKNOWN, image));
   }
   else if (DirectX::IsPlanar(format)) {
      DirectX::ScratchImage planar;
      std::swap(image, planar);

      throw_if_failed(DirectX::ConvertToSinglePlane(planar.GetImages(),
                                                    planar.GetImageCount(),
                                                    planar.GetMetadata(), image));
   }

   if (const auto format = image.GetMetadata().format;
       DirectX::IsPacked(format) || DirectX::IsVideo(format)) {
      DirectX::ScratchImage old;
      std::swap(image, old);

      throw_if_failed(
         DirectX::Convert(old.GetImages(), old.GetImageCount(),
                          old.GetMetadata(), DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
                          DirectX::TEX_FILTER_DEFAULT | DirectX::TEX_FILTER_FORCE_NON_WIC,
                          1.0f, image));
   }

   return image;
}

auto get_builtin_texture(const std::string& tex_name) -> DirectX::ScratchImage
{
   const static std::unordered_map<std::string, std::uint32_t>
      builtins{{"$null_normalmap"s, 0xffff7f7f},
               {"$null_ao"s, 0xffffffff},
               {"$null_detailmap", 0xff7f7f7f},
               {"$null_heightmap", 0xff7f7f7f},
               {"$null_aomap"s, 0xffffffff},
               {"$null_metallic_roughnessmap", 0xffffffff},
               {"$null_albedomap", 0xffffffff},
               {"$null_diffusemap", 0xffffffff},
               {"$null_glossmap", 0x00000000}};

   if (const auto it = builtins.find(tex_name); it != builtins.end()) {
      DirectX::ScratchImage image;
      image.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 1);

      std::memcpy(image.GetPixels(), &it->second, sizeof(it->second));

      return image;
   }
   else {
      throw compose_exception<std::runtime_error>("Invalid texture builtin "sv,
                                                  tex_name, "!"sv);
   }
}

template<typename Accessor>
auto assemble_textures(Accessor&& accessor, const Terrain_materials_config& config,
                       const std::vector<std::string>& materials,
                       const std::filesystem::path& sptex_files_dir)
   -> std::vector<DirectX::ScratchImage>
{
   std::vector<DirectX::ScratchImage> result;
   result.reserve(materials.size());

   for (const auto& material_name : materials) {
      const auto& material = [&] {
         const auto it = config.materials.find(view_as_ci_string(material_name));

         if (it == config.materials.cend()) {
            throw compose_exception<std::runtime_error>(
               "Failed to find material ", std::quoted(material_name),
               " in terrain material config!");
         }

         return it->second;
      }();

      const auto& tex_name = std::invoke(std::forward<Accessor>(accessor), material);

      if (tex_name.front() == '$') {
         result.emplace_back(get_builtin_texture(tex_name));
      }
      else {
         result.emplace_back(load_sptex(
            (std::filesystem::path{sptex_files_dir} / tex_name) += ".sptex"sv));
      }
   }

   return result;
}

auto get_texture_array_size(
   std::initializer_list<std::reference_wrapper<std::vector<DirectX::ScratchImage>>> texture_vectors) noexcept
   -> std::array<UINT, 2>
{
   UINT w = 0;
   UINT h = 0;

   for (auto vec : texture_vectors) {
      for (const auto& img : vec.get()) {
         w = safe_max(w, static_cast<UINT>(img.GetMetadata().width));
         h = safe_max(h, static_cast<UINT>(img.GetMetadata().height));
      }
   }

   return {next_power_of_2(w), next_power_of_2(h)};
}

auto resize_textures(std::vector<DirectX::ScratchImage> input, const UINT w,
                     const UINT h) -> std::vector<DirectX::ScratchImage>
{
   std::vector<DirectX::ScratchImage> result;
   result.reserve(input.size());

   for (auto& img : input) {
      if (img.GetMetadata().width != w || img.GetMetadata().height != h) {
         throw_if_failed(DirectX::Resize(*img.GetImage(0, 0, 0), w, h,
                                         DirectX::TEX_FILTER_DEFAULT |
                                            DirectX::TEX_FILTER_FORCE_NON_WIC,
                                         result.emplace_back()));
      }
      else {
         result.emplace_back(std::move(img));
      }
   }

   return result;
}

auto pack_textures(const std::vector<DirectX::ScratchImage>& left_textures,
                   const glm::ivec4 left_mapping,
                   const std::vector<DirectX::ScratchImage>& right_textures,
                   const glm::ivec4 right_mapping,
                   const DXGI_FORMAT packed_format) -> DirectX::ScratchImage
{
   Expects(left_textures.size() == right_textures.size());
   Expects(glm::all(glm::lessThan(left_mapping, glm::ivec4{glm::vec4::length()})));
   Expects(glm::all(glm::lessThan(right_mapping, glm::ivec4{glm::vec4::length()})));

   DirectX::ScratchImage packed_texture;
   packed_texture.Initialize2D(packed_format, left_textures[0].GetMetadata().width,
                               left_textures[0].GetMetadata().height,
                               left_textures.size(), 1);

   for (auto i = 0; i < right_textures.size(); ++i) {
      Image_span left{*left_textures[i].GetImage(0, 0, 0)};
      Image_span right{*right_textures[i].GetImage(0, 0, 0)};
      Image_span packed{*packed_texture.GetImage(0, i, 0)};

      for_each(std::execution::par_unseq, left, [&](const glm::ivec2 index) {
         const auto left_val = left.load(index);
         const auto right_val = right.load(index);

         glm::vec4 packed_val{0.0f, 0.0f, 0.0f, 1.0f};

         // clang-format off
         if (left_mapping[0] >= 0) packed_val[left_mapping[0]] = left_val[0];
         if (left_mapping[1] >= 0) packed_val[left_mapping[1]] = left_val[1];
         if (left_mapping[2] >= 0) packed_val[left_mapping[2]] = left_val[2];
         if (left_mapping[3] >= 0) packed_val[left_mapping[3]] = left_val[3];
         if (right_mapping[0] >= 0) packed_val[right_mapping[0]] = right_val[0];
         if (right_mapping[1] >= 0) packed_val[right_mapping[1]] = right_val[1];
         if (right_mapping[2] >= 0) packed_val[right_mapping[2]] = right_val[2];
         if (right_mapping[3] >= 0) packed_val[right_mapping[3]] = right_val[3];
         // clang-format on

         packed.store(index, packed_val);
      });
   }

   return packed_texture;
}

auto pack_textures(const std::vector<DirectX::ScratchImage>& textures,
                   const DXGI_FORMAT packed_format) -> DirectX::ScratchImage
{
   Expects(!textures.empty());

   DirectX::ScratchImage packed_texture;
   packed_texture.Initialize2D(packed_format, textures.front().GetMetadata().width,
                               textures.front().GetMetadata().height,
                               textures.size(), 1);

   for (auto i = 0; i < textures.size(); ++i) {
      Image_span src{*textures[i].GetImage(0, 0, 0)};
      Image_span dest{*packed_texture.GetImage(0, i, 0)};

      for_each(std::execution::par_unseq, src, [&](const glm::ivec2 index) {
         dest.store(index, src.load(index));
      });
   }

   return packed_texture;
}

auto generate_mipmaps(DirectX::ScratchImage image) -> DirectX::ScratchImage
{
   if (image.GetMetadata().width == 1 && image.GetMetadata().height == 1) {
      return image;
   }

   DirectX::ScratchImage result;

   throw_if_failed(DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(),
                                            image.GetMetadata(),
                                            DirectX::TEX_FILTER_DEFAULT |
                                               DirectX::TEX_FILTER_FORCE_NON_WIC,
                                            0, result));

   return result;
}

auto compress_texture(DirectX::ScratchImage image, const DXGI_FORMAT format,
                      ID3D11Device* d3d11) -> DirectX::ScratchImage
{
   if (image.GetMetadata().width < 4 && image.GetMetadata().height < 4) {
      return image;
   }

   const auto compress_flags =
      DirectX::TEX_COMPRESS_PARALLEL | DirectX::TEX_COMPRESS_UNIFORM;

   DirectX::ScratchImage result;

   if (DirectX::MakeTypeless(format) == DXGI_FORMAT_BC7_TYPELESS ||
       DirectX::MakeTypeless(format) == DXGI_FORMAT_BC6H_TYPELESS && d3d11) {
      throw_if_failed(DirectX::Compress(d3d11, image.GetImages(),
                                        image.GetImageCount(), image.GetMetadata(),
                                        format, compress_flags, 1.0f, result));
   }
   else {
      throw_if_failed(DirectX::Compress(image.GetImages(), image.GetImageCount(),
                                        image.GetMetadata(), format,
                                        compress_flags, 1.0f, result));
   }

   return result;
}

void save_texture(const std::filesystem::path output_path,
                  const DirectX::ScratchImage& image)
{
   auto meta = image.GetMetadata();

   Texture_info info;
   info.type = Texture_type::texture2d;
   info.width = static_cast<std::uint32_t>(meta.width);
   info.height = static_cast<std::uint32_t>(meta.height);
   info.depth = 1;
   info.array_size = static_cast<std::uint32_t>(meta.arraySize);
   info.mip_count = static_cast<std::uint32_t>(meta.mipLevels);
   info.format = meta.format;

   std::vector<Texture_data> texture_data;
   texture_data.reserve(meta.mipLevels * meta.arraySize);

   for (auto index = 0; index < meta.arraySize; ++index) {
      for (auto mip = 0; mip < meta.mipLevels; ++mip) {
         auto* const sub_image = image.GetImage(mip, index, 0);

         Texture_data data;
         data.pitch = gsl::narrow_cast<UINT>(sub_image->rowPitch);
         data.slice_pitch = gsl::narrow_cast<UINT>(sub_image->slicePitch);
         data.data = gsl::make_span(reinterpret_cast<std::byte*>(sub_image->pixels),
                                    sub_image->slicePitch);

         texture_data.emplace_back(data);
      }
   }

   write_patch_texture(output_path, info, texture_data,
                       Texture_file_type::volume_resource);
}

void terrain_assemble_textures_pbr(const Terrain_materials_config& config,
                                   const std::string_view texture_suffix,
                                   const std::vector<std::string>& materials,
                                   const std::filesystem::path& output_munge_files_dir,
                                   const std::filesystem::path& sptex_files_dir)
{
   const auto albedo_ao_maps_paths =
      output_munge_files_dir / ((std::string{terrain_albedo_ao_texture_name} +=
                                 texture_suffix) += ".sptex"sv);
   const auto normal_mr_maps_paths =
      output_munge_files_dir / ((std::string{terrain_normal_mr_texture_name} +=
                                 texture_suffix) += ".sptex"sv);
   const auto height_maps_paths =
      output_munge_files_dir /
      ((std::string{terrain_height_texture_name} += texture_suffix) += ".sptex"sv);

   if (!should_assemble_textures(config, sptex_files_dir,
                                 {albedo_ao_maps_paths, normal_mr_maps_paths,
                                  height_maps_paths})) {
      return;
   }

   auto d3d11 = create_d3d11_device();

   {
      auto albedo_maps = assemble_textures(
         [](const Terrain_material& mat) { return mat.albedo_map; }, config,
         materials, sptex_files_dir);
      auto ao_maps =
         assemble_textures([](const Terrain_material& mat) { return mat.ao_map; },
                           config, materials, sptex_files_dir);

      const auto [width, height] = get_texture_array_size({albedo_maps, ao_maps});

      albedo_maps = resize_textures(std::move(albedo_maps), width, height);
      ao_maps = resize_textures(std::move(ao_maps), width, height);

      const auto packed_albedo_ao_maps =
         compress_texture(generate_mipmaps(
                             pack_textures(albedo_maps, {0, 1, 2, -1}, ao_maps,
                                           {3, -1, -1, -1},
                                           DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)),
                          DXGI_FORMAT_BC7_UNORM_SRGB, d3d11.get());

      save_texture(albedo_ao_maps_paths, packed_albedo_ao_maps);
   }

   {
      auto normal_maps = assemble_textures(
         [](const Terrain_material& mat) { return mat.normal_map; }, config,
         materials, sptex_files_dir);
      auto mr_maps = assemble_textures(
         [](const Terrain_material& mat) { return mat.metallic_roughness_map; },
         config, materials, sptex_files_dir);

      const auto [width, height] = get_texture_array_size({normal_maps, mr_maps});

      normal_maps = resize_textures(std::move(normal_maps), width, height);
      mr_maps = resize_textures(std::move(mr_maps), width, height);

      const auto packed_normal_mr_maps =
         compress_texture(generate_mipmaps(
                             pack_textures(normal_maps, {0, 1, -1, -1}, mr_maps,
                                           {2, 3, -1, -1}, DXGI_FORMAT_R8G8B8A8_UNORM)),
                          DXGI_FORMAT_BC7_UNORM, d3d11.get());

      save_texture(normal_mr_maps_paths, packed_normal_mr_maps);
   }

   {
      auto height_maps = assemble_textures(
         [](const Terrain_material& mat) { return mat.height_map; }, config,
         materials, sptex_files_dir);

      const auto [width, height] = get_texture_array_size({height_maps});

      height_maps = resize_textures(std::move(height_maps), width, height);

      const auto packed_height_maps =
         compress_texture(generate_mipmaps(pack_textures(height_maps, DXGI_FORMAT_R8_UNORM)),
                          DXGI_FORMAT_BC4_UNORM, d3d11.get());

      save_texture(height_maps_paths, packed_height_maps);
   }
}

void terrain_assemble_textures_normal_ext(const Terrain_materials_config& config,
                                          const std::string_view texture_suffix,
                                          const std::vector<std::string>& materials,
                                          const std::filesystem::path& output_munge_files_dir,
                                          const std::filesystem::path& sptex_files_dir)
{
   const auto diffuse_ao_maps_paths =
      output_munge_files_dir / ((std::string{terrain_diffuse_ao_texture_name} +=
                                 texture_suffix) += ".sptex"sv);
   const auto normal_gloss_maps_paths =
      output_munge_files_dir / ((std::string{terrain_normal_gloss_texture_name} +=
                                 texture_suffix) += ".sptex"sv);
   const auto height_maps_paths =
      output_munge_files_dir /
      ((std::string{terrain_height_texture_name} += texture_suffix) += ".sptex"sv);

   if (!should_assemble_textures(config, sptex_files_dir,
                                 {diffuse_ao_maps_paths,
                                  normal_gloss_maps_paths, height_maps_paths})) {
      return;
   }

   auto d3d11 = create_d3d11_device();

   {
      auto diffuse_maps = assemble_textures(
         [](const Terrain_material& mat) { return mat.diffuse_map; }, config,
         materials, sptex_files_dir);
      auto ao_maps =
         assemble_textures([](const Terrain_material& mat) { return mat.ao_map; },
                           config, materials, sptex_files_dir);

      const auto [width, height] = get_texture_array_size({diffuse_maps, ao_maps});

      diffuse_maps = resize_textures(std::move(diffuse_maps), width, height);
      ao_maps = resize_textures(std::move(ao_maps), width, height);

      const auto packed_diffuse_ao_maps =
         compress_texture(generate_mipmaps(
                             pack_textures(diffuse_maps, {0, 1, 2, -1}, ao_maps,
                                           {3, -1, -1, -1},
                                           DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)),
                          config.srgb_diffuse_maps ? DXGI_FORMAT_BC7_UNORM_SRGB
                                                   : DXGI_FORMAT_BC7_UNORM,
                          d3d11.get());

      save_texture(diffuse_ao_maps_paths, packed_diffuse_ao_maps);
   }

   {
      auto normal_maps = assemble_textures(
         [](const Terrain_material& mat) { return mat.normal_map; }, config,
         materials, sptex_files_dir);
      auto gloss_maps = assemble_textures(
         [](const Terrain_material& mat) { return mat.gloss_map; }, config,
         materials, sptex_files_dir);

      const auto [width, height] = get_texture_array_size({normal_maps, gloss_maps});

      normal_maps = resize_textures(std::move(normal_maps), width, height);
      gloss_maps = resize_textures(std::move(gloss_maps), width, height);

      const auto packed_normal_gloss_maps =
         compress_texture(generate_mipmaps(
                             pack_textures(normal_maps, {0, 1, -1, -1}, gloss_maps,
                                           {2, -1, -1, -1}, DXGI_FORMAT_R8G8B8A8_UNORM)),
                          DXGI_FORMAT_BC7_UNORM, d3d11.get());

      save_texture(normal_gloss_maps_paths, packed_normal_gloss_maps);
   }

   {
      auto height_maps = assemble_textures(
         [](const Terrain_material& mat) { return mat.height_map; }, config,
         materials, sptex_files_dir);

      const auto [width, height] = get_texture_array_size({height_maps});

      height_maps = resize_textures(std::move(height_maps), width, height);

      const auto packed_height_maps =
         compress_texture(generate_mipmaps(pack_textures(height_maps, DXGI_FORMAT_R8_UNORM)),
                          DXGI_FORMAT_BC4_UNORM, d3d11.get());

      save_texture(height_maps_paths, packed_height_maps);
   }
}
}

void terrain_assemble_textures(const Terrain_materials_config& config,
                               const std::string_view texture_suffix,
                               const std::vector<std::string>& materials,
                               const std::filesystem::path& output_munge_files_dir,
                               const std::filesystem::path& sptex_files_dir)
{
   if (materials.empty()) return;

   if (config.rendertype == Terrain_rendertype::pbr) {
      terrain_assemble_textures_pbr(config, texture_suffix, materials,
                                    output_munge_files_dir, sptex_files_dir);
   }
   else if (config.rendertype == Terrain_rendertype::normal_ext) {
      terrain_assemble_textures_normal_ext(config, texture_suffix, materials,
                                           output_munge_files_dir, sptex_files_dir);
   }
}
}
