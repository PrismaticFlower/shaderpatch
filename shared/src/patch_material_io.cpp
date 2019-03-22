
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
enum class Material_version : std::uint32_t { v_1, current = v_1 };

inline namespace v_1 {
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
      writer.emplace_child("CBST"_mn).write(info.cb_shader_stages);
      writer.emplace_child("CB__"_mn).write(gsl::make_span(info.constant_buffer));

      const auto write_textures = [&](const Magic_number mn,
                                      const std::vector<std::string>& textures) {
         auto texs = writer.emplace_child(mn);

         texs.write<std::uint32_t>(gsl::narrow_cast<std::uint32_t>(textures.size()));
         for (const auto& texture : textures) texs.write(texture);
      };

      write_textures("VSSR"_mn, info.vs_textures);
      write_textures("HSSR"_mn, info.hs_textures);
      write_textures("DSSR"_mn, info.ds_textures);
      write_textures("GSSR"_mn, info.gs_textures);
      write_textures("PSSR"_mn, info.ps_textures);

      writer.emplace_child("FSTX"_mn).write(info.fail_safe_texture_index);
      writer.emplace_child("TESS"_mn).write(info.tessellation,
                                            info.tessellation_primitive_topology);
   }

   const auto matl_data = ostream.str();
   const auto matl_span =
      gsl::span<const std::byte>(reinterpret_cast<const std::byte*>(matl_data.data()),
                                 matl_data.size());

   save_volume_resource(save_path.string(), save_path.stem().string(),
                        Volume_resource_type::material, matl_span);
}

auto read_patch_material(ucfb::Reader_strict<"matl"_mn> reader) -> Material_info
{
   const auto version =
      reader.read_child_strict<"VER_"_mn>().read<Material_version>();

   reader.reset_head();

   switch (version) {
   case Material_version::current:
      return read_patch_material_impl(reader);
   default:
      throw std::runtime_error{"material has newer version than supported by "
                               "this version of Shader Patch!"};
   }
}

namespace {

namespace v_1 {
auto read_patch_material_impl(ucfb::Reader reader) -> Material_info
{
   Material_info info{};

   const auto version =
      reader.read_child_strict<"VER_"_mn>().read<Material_version>();

   Ensures(version == Material_version::v_1);

   info.name = reader.read_child_strict<"NAME"_mn>().read_string();
   info.rendertype = reader.read_child_strict<"RTYP"_mn>().read_string();
   info.overridden_rendertype =
      reader.read_child_strict<"ORTP"_mn>().read<Rendertype>();

   info.cb_shader_stages =
      reader.read_child_strict<"CBST"_mn>().read<Material_cb_shader_stages>();

   {
      auto cb__ = reader.read_child_strict<"CB__"_mn>();
      const auto constants = cb__.read_array<std::byte>(cb__.size());

      info.constant_buffer.resize(next_multiple_of<std::size_t{16}>(constants.size()));
      std::memcpy(info.constant_buffer.data(), constants.data(), constants.size());
   }

   const auto read_textures = [&](auto texs, std::vector<std::string>& textures) {
      const auto count = texs.read<std::uint32_t>();
      textures.reserve(count);

      for (auto i = 0; i < count; ++i)
         textures.emplace_back(texs.read_string());
   };

   read_textures(reader.read_child_strict<"VSSR"_mn>(), info.vs_textures);
   read_textures(reader.read_child_strict<"HSSR"_mn>(), info.hs_textures);
   read_textures(reader.read_child_strict<"DSSR"_mn>(), info.ds_textures);
   read_textures(reader.read_child_strict<"GSSR"_mn>(), info.gs_textures);
   read_textures(reader.read_child_strict<"PSSR"_mn>(), info.ps_textures);

   info.fail_safe_texture_index =
      reader.read_child_strict<"FSTX"_mn>().read<std::uint32_t>();

   std::tie(info.tessellation, info.tessellation_primitive_topology) =
      reader.read_child_strict<"TESS"_mn>().read_multi<bool, D3D11_PRIMITIVE_TOPOLOGY>();

   return info;
}
}

}
}
