
#include "shader_loader.hpp"
#include "logger.hpp"
#include "throw_if_failed.hpp"

#include <boost/container/flat_map.hpp>
#include <string>
#include <vector>

namespace sp {

using namespace std::literals;

namespace {

template<Magic_number mn>
void read_entrypoints(ucfb::Reader_strict<mn> reader, IDirect3DDevice9& device,
                      Shader_group& group)
{
   static_assert(mn == "VSHD"_mn || mn == "PSHD"_mn);
   using Shader_flags_type =
      std::conditional_t<mn == "VSHD"_mn, Vertex_shader_flags, Pixel_shader_flags>;
   using Entrypoint_type =
      std::conditional_t<mn == "VSHD"_mn, Vertex_shader_entrypoint, Pixel_shader_entrypoint>;
   using D3d_type =
      std::conditional_t<mn == "VSHD"_mn, IDirect3DVertexShader9, IDirect3DPixelShader9>;

   const auto count =
      reader.read_child_strict<"SIZE"_mn>().read_trivial<std::uint32_t>();

   for (auto ep_index = 0u; ep_index < count; ++ep_index) {
      auto ep_reader = reader.read_child_strict<"EP__"_mn>();
      const auto ep_name = ep_reader.read_child_strict<"NAME"_mn>().read_string();

      auto vrs_reader = ep_reader.read_child_strict<"VRS_"_mn>();

      const auto variation_count =
         vrs_reader.read_child_strict<"SIZE"_mn>().read_trivial<std::uint32_t>();

      Entrypoint_type entrypoint;

      for (auto vari_index = 0u; vari_index < variation_count; ++vari_index) {
         auto vari_reader = vrs_reader.read_child_strict<"VARI"_mn>();

         const auto flags = vari_reader.read_trivial<Shader_flags_type>();
         const auto static_flags = vari_reader.read_trivial<std::uint32_t>();
         const auto bytecode_size = vari_reader.read_trivial<std::uint32_t>();
         const auto bytecode = vari_reader.read_array<std::byte>(bytecode_size);

         Com_ptr<D3d_type> shader;

         if constexpr (mn == "VSHD"_mn) {
            throw_if_failed(
               device.CreateVertexShader(reinterpret_cast<const DWORD*>(bytecode.data()),
                                         shader.clear_and_assign()));
         }
         else if constexpr (mn == "PSHD"_mn) {
            throw_if_failed(
               device.CreatePixelShader(reinterpret_cast<const DWORD*>(bytecode.data()),
                                        shader.clear_and_assign()));
         }

         entrypoint.insert(std::move(shader),
                           gsl::narrow_cast<std::uint16_t>(static_flags), flags);
      }

      if constexpr (mn == "VSHD"_mn) {
         group.vertex.insert(std::string{ep_name}, std::move(entrypoint));
      }
      else if constexpr (mn == "PSHD"_mn) {
         group.pixel.insert(std::string{ep_name}, std::move(entrypoint));
      }
   }
}

void read_state(ucfb::Reader_strict<"STAT"_mn> reader,
                const Shader_group& shader_group, Shader_rendertype& rendertype)
{
   const auto state_name =
      std::string{reader.read_child_strict<"NAME"_mn>().read_string()};

   auto info_reader = reader.read_child_strict<"INFO"_mn>();

   const auto vs_entrypoint = std::string{info_reader.read_string()};
   const auto vs_static_flags =
      gsl::narrow_cast<std::uint16_t>(info_reader.read_trivial<std::uint32_t>());
   const auto ps_entrypoint = std::string{info_reader.read_string()};
   const auto ps_static_flags =
      gsl::narrow_cast<std::uint16_t>(info_reader.read_trivial<std::uint32_t>());

   Shader_state state{Vertex_shader_entrypoint{shader_group.vertex.at(vs_entrypoint)},
                      vs_static_flags,
                      Pixel_shader_entrypoint{shader_group.pixel.at(ps_entrypoint)},
                      ps_static_flags};

   rendertype.insert(state_name, std::move(state));
}

void read_rendertypes(ucfb::Reader_strict<"RTS_"_mn> reader,
                      const Shader_group& shader_group,
                      Shader_rendertype_collection& rendertype_collection)
{
   const auto count =
      reader.read_child_strict<"SIZE"_mn>().read_trivial<std::uint32_t>();

   for (auto rt_index = 0u; rt_index < count; ++rt_index) {
      auto rtyp_reader = reader.read_child_strict<"RTYP"_mn>();

      const auto rendertype_name =
         std::string{rtyp_reader.read_child_strict<"NAME"_mn>().read_string()};
      const auto state_count =
         rtyp_reader.read_child_strict<"SIZE"_mn>().read_trivial<std::uint32_t>();

      Shader_rendertype rendertype;

      for (auto state_index = 0u; state_index < state_count; ++state_index) {
         read_state(rtyp_reader.read_child_strict<"STAT"_mn>(), shader_group, rendertype);
      }

      rendertype_collection.insert(rendertype_name, std::move(rendertype));
   }
}

}

void load_shader_pack(ucfb::Reader_strict<"shpk"_mn> reader,
                      IDirect3DDevice9& device, Shader_database& database)
{
   const auto group_name =
      std::string{reader.read_child_strict<"NAME"_mn>().read_string()};

   if (database.groups.find(group_name)) return;

   Shader_group shader_group;

   auto entrypoints_reader = reader.read_child_strict<"EPTS"_mn>();

   read_entrypoints(entrypoints_reader.read_child_strict<"VSHD"_mn>(), device,
                    shader_group);
   read_entrypoints(entrypoints_reader.read_child_strict<"PSHD"_mn>(), device,
                    shader_group);

   read_rendertypes(reader.read_child_strict<"RTS_"_mn>(), shader_group,
                    database.rendertypes);

   database.groups.insert(group_name, std::move(shader_group));
}
}
