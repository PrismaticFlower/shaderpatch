#pragma once

#include "ucfb_writer.hpp"
#include "volume_resource.hpp"

#include <array>
#include <cstdint>
#include <sstream>
#include <string>

#include <glm/glm.hpp>

#include <boost/filesystem.hpp>

namespace sp {

enum class Material_version : std::uint32_t { _1 };

struct Material_info {
   std::string rendertype;
   std::string overridden_rendertype;
   std::array<glm::vec4, 8> constants{};
   std::array<std::string, 8> textures{};
};

inline void write_patch_material(boost::filesystem::path save_path,
                                 const Material_info& info)
{
   using namespace std::literals;
   namespace fs = boost::filesystem;

   std::ostringstream ostream;

   ucfb::Writer writer{ostream, "matl"_mn};

   writer.emplace_child("VER_"_mn).write(Material_version::_1);

   writer.emplace_child("RTYP"_mn).write(info.rendertype);
   writer.emplace_child("ORTP"_mn).write(info.overridden_rendertype);
   writer.emplace_child("CNST"_mn).write(gsl::make_span(info.constants));
   writer.emplace_child("TX05"_mn).write(info.textures[0]);
   writer.emplace_child("TX06"_mn).write(info.textures[1]);
   writer.emplace_child("TX07"_mn).write(info.textures[2]);
   writer.emplace_child("TX08"_mn).write(info.textures[3]);
   writer.emplace_child("TX09"_mn).write(info.textures[4]);
   writer.emplace_child("TX10"_mn).write(info.textures[5]);
   writer.emplace_child("TX11"_mn).write(info.textures[6]);
   writer.emplace_child("TX12"_mn).write(info.textures[7]);

   const auto matl_data = ostream.str();
   const auto matl_span =
      gsl::span<const std::byte>(reinterpret_cast<const std::byte*>(matl_data.data()),
                                 matl_data.size());

   save_volume_resource(save_path.string(), save_path.stem().string(),
                        Volume_resource_type::material, matl_span);
}
}
