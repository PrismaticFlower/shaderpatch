
#include "patch_material_io.hpp"
#include "game_rendertypes.hpp"
#include "ucfb_reader.hpp"
#include "ucfb_writer.hpp"
#include "volume_resource.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>

#include <gsl/gsl>

namespace sp {

namespace {
enum class Material_version : std::uint32_t {
   _1_0_0,
   _2_0_0,
   _3_0_0,
   _4_0_0,
   current = _4_0_0
};

inline namespace v_4_0_0 {
auto read_patch_material_impl(ucfb::Reader reader) -> Material_info;
}

namespace v_3_0_0 {
auto read_patch_material_impl(ucfb::Reader reader) -> Material_info;
}

namespace v_2_0_0 {
auto read_patch_material_impl(ucfb::Reader reader) -> Material_info;
}

namespace v_1_0_0 {
auto read_patch_material_impl(ucfb::Reader reader) -> Material_info;
}
}

void write_patch_material(const std::filesystem::path& save_path,
                          const Material_info& info)
{
   using namespace std::literals;
   namespace fs = std::filesystem;

   std::ostringstream ostream;

   // write material chunk
   {
      ucfb::Writer writer{ostream, "matl"_mn};

      writer.emplace_child("VER_"_mn).write(Material_version::current);

      writer.emplace_child("NAME"_mn).write(info.name);
      writer.emplace_child("RTYP"_mn).write(info.rendertype);
      writer.emplace_child("ORTP"_mn).write(info.overridden_rendertype);
      writer.emplace_child("CNST"_mn).write(gsl::make_span(info.constant_buffer));

      {
         auto texs = writer.emplace_child("TEXS"_mn);

         texs.write<std::uint32_t>(
            gsl::narrow_cast<std::uint32_t>(info.textures.size()));
         for (const auto& texture : info.textures) texs.write(texture);
      }

      writer.emplace_child("FSTX"_mn).write(info.fail_safe_texture_index);
   }

   const auto matl_data = ostream.str();
   const auto matl_span =
      gsl::span<const std::byte>(reinterpret_cast<const std::byte*>(matl_data.data()),
                                 matl_data.size());

   save_volume_resource(save_path.string(), save_path.stem().string(),
                        Volume_resource_type::material, matl_span);
}

auto read_patch_material(ucfb::Reader reader) -> Material_info
{
   const auto version =
      reader.read_child_strict<"VER_"_mn>().read<Material_version>();

   reader.reset_head();

   switch (version) {
   case Material_version::current:
      return read_patch_material_impl(reader);
   case Material_version::_3_0_0:
      return v_3_0_0::read_patch_material_impl(reader);
   case Material_version::_2_0_0:
      return v_2_0_0::read_patch_material_impl(reader);
   case Material_version::_1_0_0:
      return v_1_0_0::read_patch_material_impl(reader);
   default:
      throw std::runtime_error{"material has unknown version"};
   }
}

namespace {
namespace legacy {
void trim_textures(std::vector<std::string>& textures) noexcept
{
   for (auto i = static_cast<gsl::index>(textures.size()) - 1; i >= 0; --i) {
      if (!textures[i].empty()) break;

      textures.pop_back();
   }
}

auto fail_safe_texture_index(const std::string_view rendertype) noexcept -> std::int32_t
{
   using namespace std::literals;

   if (rendertype == "pbr"sv) return 0;
   if (rendertype == "pbr.basic"sv) return 0;
   if (rendertype == "pbr.emissive"sv) return 0;
   if (rendertype == "normal_ext"sv) return 0;
   if (rendertype == "normal_ext.detail"sv) return 0;
   if (rendertype == "normal_ext.specular"sv) return 0;
   if (rendertype == "normal_ext.specular.detail"sv) return 0;

   return -1;
}
}

namespace v_4_0_0 {
auto read_patch_material_impl(ucfb::Reader reader) -> Material_info
{
   Material_info info{};

   const auto version =
      reader.read_child_strict<"VER_"_mn>().read<Material_version>();

   Ensures(version == Material_version::_4_0_0);

   info.name = reader.read_child_strict<"NAME"_mn>().read_string();
   info.rendertype = reader.read_child_strict<"RTYP"_mn>().read_string();
   info.overridden_rendertype =
      reader.read_child_strict<"ORTP"_mn>().read<Rendertype>();

   {
      auto cnst = reader.read_child_strict<"CNST"_mn>();
      const auto constants = cnst.read_array<std::byte>(cnst.size());

      info.constant_buffer.resize(next_multiple_of<std::size_t{16}>(constants.size()));
      std::memcpy(info.constant_buffer.data(), constants.data(), constants.size());
   }

   {
      auto texs = reader.read_child_strict<"TEXS"_mn>();

      const auto count = texs.read<std::uint32_t>();
      info.textures.reserve(count);

      for (auto i = 0; i < count; ++i)
         info.textures.emplace_back(texs.read_string());
   }

   info.fail_safe_texture_index =
      reader.read_child_strict<"FSTX"_mn>().read<std::uint32_t>();

   return info;
}
}

namespace v_3_0_0 {
auto read_patch_material_impl(ucfb::Reader reader) -> Material_info
{
   Material_info info{};

   const auto version =
      reader.read_child_strict<"VER_"_mn>().read<Material_version>();

   Ensures(version == Material_version::_3_0_0);

   info.name = reader.read_child_strict<"NAME"_mn>().read_string();
   info.rendertype = reader.read_child_strict<"RTYP"_mn>().read_string();
   info.overridden_rendertype =
      reader.read_child_strict<"ORTP"_mn>().read<Rendertype>();

   const auto constants = reader.read_child_strict<"CNST"_mn>().read_array<std::byte>(
      sizeof(std::array<float, 32>));
   info.constant_buffer.assign(constants.cbegin(), constants.cend());

   // Discard Texture Size Mappings
   reader.read_child_strict<"TXSM"_mn>().read<std::array<std::int32_t, 8>>();

   info.textures.emplace_back(reader.read_child_strict<"TX04"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX05"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX06"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX07"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX08"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX09"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX10"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX11"_mn>().read_string());

   legacy::trim_textures(info.textures);

   info.fail_safe_texture_index = legacy::fail_safe_texture_index(info.rendertype);

   return info;
}
}

namespace v_2_0_0 {
auto read_patch_material_impl(ucfb::Reader reader) -> Material_info
{
   Material_info info{};

   const auto version =
      reader.read_child_strict<"VER_"_mn>().read<Material_version>();

   Ensures(version == Material_version::_2_0_0);

   info.name = "OldUnnamedMaterial";
   info.rendertype = reader.read_child_strict<"RTYP"_mn>().read_string();
   info.overridden_rendertype =
      rendertype_from_string(reader.read_child_strict<"ORTP"_mn>().read_string());

   const auto constants = reader.read_child_strict<"CNST"_mn>().read_array<std::byte>(
      sizeof(std::array<float, 32>));
   info.constant_buffer.assign(constants.cbegin(), constants.cend());

   // Discard Texture Size Mappings
   reader.read_child_strict<"TXSM"_mn>().read<std::array<std::int32_t, 8>>();

   info.textures.emplace_back(reader.read_child_strict<"TX04"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX05"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX06"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX07"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX08"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX09"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX10"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX11"_mn>().read_string());

   legacy::trim_textures(info.textures);

   info.fail_safe_texture_index = legacy::fail_safe_texture_index(info.rendertype);

   return info;
}
}

namespace v_1_0_0 {
auto read_patch_material_impl(ucfb::Reader reader) -> Material_info
{
   Material_info info{};

   const auto version =
      reader.read_child_strict<"VER_"_mn>().read<Material_version>();

   Ensures(version == Material_version::_1_0_0);

   info.name = "OldUnnamedMaterial";
   info.rendertype = reader.read_child_strict<"RTYP"_mn>().read_string();
   info.overridden_rendertype =
      rendertype_from_string(reader.read_child_strict<"ORTP"_mn>().read_string());

   const auto constants = reader.read_child_strict<"CNST"_mn>().read_array<std::byte>(
      sizeof(std::array<float, 32>));
   info.constant_buffer.assign(constants.cbegin(), constants.cend());

   info.textures.emplace_back(reader.read_child_strict<"TX04"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX05"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX06"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX07"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX08"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX09"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX10"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX11"_mn>().read_string());

   legacy::trim_textures(info.textures);

   info.fail_safe_texture_index = legacy::fail_safe_texture_index(info.rendertype);

   return info;
}
}

}
}
