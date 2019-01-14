#pragma once

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

#include <glm/glm.hpp>
#include <gsl/gsl>

namespace sp {

enum class Material_version : std::uint32_t {
   _1_0_0,
   _2_0_0,
   _3_0_0,
   current = _3_0_0
};

struct Material_info {
   std::string name;
   std::string rendertype;
   Rendertype overridden_rendertype;
   std::vector<std::byte> constant_buffer{};
   std::vector<std::string> textures{};
};

namespace materials {
inline namespace v_3_0_0 {
inline auto read_patch_material(ucfb::Reader reader) -> Material_info;
}

namespace v_2_0_0 {
inline auto read_patch_material(ucfb::Reader reader) -> Material_info;
}

namespace v_1_0_0 {
inline auto read_patch_material(ucfb::Reader reader) -> Material_info;
}
}

// inline void write_patch_material(const std::filesystem::path& save_path,
//                                  const Material_info& info)
// {
//    using namespace std::literals;
//    namespace fs = std::filesystem;
//
//    std::ostringstream ostream;
//
//    // write material chunk
//    {
//       ucfb::Writer writer{ostream, "matl"_mn};
//
//       writer.emplace_child("VER_"_mn).write(Material_version::current);
//
//       writer.emplace_child("NAME"_mn).write(info.name);
//       writer.emplace_child("RTYP"_mn).write(info.rendertype);
//       writer.emplace_child("ORTP"_mn).write(info.overridden_rendertype);
//       writer.emplace_child("CNST"_mn).write(gsl::make_span(info.constants));
//       writer.emplace_child("TXSM"_mn).write(gsl::make_span(info.texture_size_mappings));
//       writer.emplace_child("TX04"_mn).write(info.textures[0]);
//       writer.emplace_child("TX05"_mn).write(info.textures[1]);
//       writer.emplace_child("TX06"_mn).write(info.textures[2]);
//       writer.emplace_child("TX07"_mn).write(info.textures[3]);
//       writer.emplace_child("TX08"_mn).write(info.textures[4]);
//       writer.emplace_child("TX09"_mn).write(info.textures[5]);
//       writer.emplace_child("TX10"_mn).write(info.textures[6]);
//       writer.emplace_child("TX11"_mn).write(info.textures[7]);
//    }
//
//    const auto matl_data = ostream.str();
//    const auto matl_span =
//       gsl::span<const std::byte>(reinterpret_cast<const std::byte*>(matl_data.data()),
//                                  matl_data.size());
//
//    save_volume_resource(save_path.string(), save_path.stem().string(),
//                         Volume_resource_type::material, matl_span);
// }

inline auto read_patch_material(ucfb::Reader reader) -> Material_info
{
   const auto version =
      reader.read_child_strict<"VER_"_mn>().read_trivial<Material_version>();

   reader.reset_head();

   switch (version) {
   case Material_version::current:
      return materials::read_patch_material(reader);
   case Material_version::_2_0_0:
      return materials::v_2_0_0::read_patch_material(reader);
   case Material_version::_1_0_0:
      return materials::v_1_0_0::read_patch_material(reader);
   default:
      throw std::runtime_error{"material has unknown version"};
   }
}

namespace materials {

inline void trim_textures(std::vector<std::string>& textures) noexcept
{
   for (auto i = static_cast<gsl::index>(textures.size()) - 1; i >= 0; --i) {
      if (textures[i].empty()) break;

      textures.pop_back();
   }
}

namespace v_3_0_0 {
inline auto read_patch_material(ucfb::Reader reader) -> Material_info
{
   Material_info info{};

   const auto version =
      reader.read_child_strict<"VER_"_mn>().read_trivial<Material_version>();

   Ensures(version == Material_version::_3_0_0);

   info.name = reader.read_child_strict<"NAME"_mn>().read_string();
   info.rendertype = reader.read_child_strict<"RTYP"_mn>().read_string();
   info.overridden_rendertype =
      reader.read_child_strict<"ORTP"_mn>().read_trivial<Rendertype>();

   const auto constants_span =
      reader.read_child_strict<"CNST"_mn>().read_array<std::byte>(
         sizeof(std::array<float, 32>));
   info.constant_buffer.assign(constants_span.cbegin(), constants_span.cend());

   // Discard Texture Size Mappings
   reader.read_child_strict<"TXSM"_mn>().read_trivial<std::array<std::int32_t, 8>>();

   info.textures.emplace_back(reader.read_child_strict<"TX04"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX05"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX06"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX07"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX08"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX09"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX10"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX11"_mn>().read_string());

   trim_textures(info.textures);

   return info;
}
}

namespace v_2_0_0 {
inline auto read_patch_material(ucfb::Reader reader) -> Material_info
{
   Material_info info{};

   const auto version =
      reader.read_child_strict<"VER_"_mn>().read_trivial<Material_version>();

   Ensures(version == Material_version::_2_0_0);

   info.name = "OldUnnamedMaterial";
   info.rendertype = reader.read_child_strict<"RTYP"_mn>().read_string();
   info.overridden_rendertype =
      rendertype_from_string(reader.read_child_strict<"ORTP"_mn>().read_string());

   const auto constants_span =
      reader.read_child_strict<"CNST"_mn>().read_array<std::byte>(
         sizeof(std::array<float, 32>));
   info.constant_buffer.assign(constants_span.cbegin(), constants_span.cend());

   // Discard Texture Size Mappings
   reader.read_child_strict<"TXSM"_mn>().read_trivial<std::array<std::int32_t, 8>>();

   info.textures.emplace_back(reader.read_child_strict<"TX04"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX05"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX06"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX07"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX08"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX09"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX10"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX11"_mn>().read_string());

   trim_textures(info.textures);

   return info;
}
}

namespace v_1_0_0 {
inline auto read_patch_material(ucfb::Reader reader) -> Material_info
{
   Material_info info{};

   const auto version =
      reader.read_child_strict<"VER_"_mn>().read_trivial<Material_version>();

   Ensures(version == Material_version::_1_0_0);

   info.name = "OldUnnamedMaterial";
   info.rendertype = reader.read_child_strict<"RTYP"_mn>().read_string();
   info.overridden_rendertype =
      rendertype_from_string(reader.read_child_strict<"ORTP"_mn>().read_string());

   const auto constants_span =
      reader.read_child_strict<"CNST"_mn>().read_array<std::byte>(
         sizeof(std::array<float, 32>));
   info.constant_buffer.assign(constants_span.cbegin(), constants_span.cend());

   info.textures.emplace_back(reader.read_child_strict<"TX04"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX05"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX06"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX07"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX08"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX09"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX10"_mn>().read_string());
   info.textures.emplace_back(reader.read_child_strict<"TX11"_mn>().read_string());

   trim_textures(info.textures);

   return info;
}
}
}
}
