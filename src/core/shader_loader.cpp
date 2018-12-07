
#include "shader_loader.hpp"
#include "../logger.hpp"
#include "memory_mapped_file.hpp"
#include "throw_if_failed.hpp"
#include "ucfb_reader.hpp"

#include <string>
#include <vector>

#include <d3d11_1.h>

namespace sp::core {

using namespace std::literals;

namespace {

void read_input_layout(ucfb::Reader_strict<"IALO"_mn> reader,
                       const gsl::span<const std::byte> bytecode,
                       Input_layout_factory& input_layout_factory)
{
   const auto compressed = reader.read_trivial<bool>();
   const auto elem_count = reader.read_trivial<std::uint32_t>();

   std::vector<D3D11_INPUT_ELEMENT_DESC> layout;
   layout.resize(elem_count);

   for (auto& elem : layout) {
      elem.SemanticName = reader.read_string().data();
      elem.SemanticIndex = reader.read_trivial<std::uint32_t>();
      elem.Format = reader.read_trivial<DXGI_FORMAT>();
      elem.InputSlot = 0;
      elem.AlignedByteOffset = reader.read_trivial<std::uint32_t>();
      elem.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
   }

   Com_ptr<ID3D11InputLayout> input_layout;

   if (!input_layout_factory.try_add(gsl::make_span(layout), bytecode, compressed)) {
      throw std::runtime_error{"Failed to create input layout for shader."};
   }
}

void read_entrypoints(ucfb::Reader_strict<"VSHD"_mn> reader, ID3D11Device& device,
                      Shader_group& group, Input_layout_factory& input_layout_cache)
{
   const auto count =
      reader.read_child_strict<"SIZE"_mn>().read_trivial<std::uint32_t>();

   for (auto ep_index = 0u; ep_index < count; ++ep_index) {
      auto ep_reader = reader.read_child_strict<"EP__"_mn>();
      const auto ep_name = ep_reader.read_child_strict<"NAME"_mn>().read_string();

      auto vrs_reader = ep_reader.read_child_strict<"VRS_"_mn>();

      const auto variation_count =
         vrs_reader.read_child_strict<"SIZE"_mn>().read_trivial<std::uint32_t>();

      Vertex_shader_entrypoint entrypoint;

      for (auto vari_index = 0u; vari_index < variation_count; ++vari_index) {
         auto vari_reader = vrs_reader.read_child_strict<"VARI"_mn>();

         const auto flags = vari_reader.read_trivial<Vertex_shader_flags>();
         const auto static_flags = vari_reader.read_trivial<std::uint32_t>();
         const auto bytecode_size = vari_reader.read_trivial<std::uint32_t>();
         const auto bytecode = vari_reader.read_array<std::byte>(bytecode_size);

         read_input_layout(vari_reader.read_child_strict<"IALO"_mn>(), bytecode,
                           input_layout_cache);

         Com_ptr<ID3D11VertexShader> shader;

         throw_if_failed(device.CreateVertexShader(bytecode.data(),
                                                   bytecode.size(), nullptr,
                                                   shader.clear_and_assign()));

         entrypoint.insert(std::move(shader),
                           gsl::narrow_cast<std::uint16_t>(static_flags), flags);
      }

      group.vertex.insert(std::string{ep_name}, std::move(entrypoint));
   }
}

void read_entrypoints(ucfb::Reader_strict<"PSHD"_mn> reader,
                      ID3D11Device& device, Shader_group& group)
{
   const auto count =
      reader.read_child_strict<"SIZE"_mn>().read_trivial<std::uint32_t>();

   for (auto ep_index = 0u; ep_index < count; ++ep_index) {
      auto ep_reader = reader.read_child_strict<"EP__"_mn>();
      const auto ep_name = ep_reader.read_child_strict<"NAME"_mn>().read_string();

      auto vrs_reader = ep_reader.read_child_strict<"VRS_"_mn>();

      const auto variation_count =
         vrs_reader.read_child_strict<"SIZE"_mn>().read_trivial<std::uint32_t>();

      Pixel_shader_entrypoint entrypoint;

      for (auto vari_index = 0u; vari_index < variation_count; ++vari_index) {
         auto vari_reader = vrs_reader.read_child_strict<"VARI"_mn>();

         const auto flags = vari_reader.read_trivial<Pixel_shader_flags>();
         const auto static_flags = vari_reader.read_trivial<std::uint32_t>();
         const auto bytecode_size = vari_reader.read_trivial<std::uint32_t>();
         const auto bytecode = vari_reader.read_array<std::byte>(bytecode_size);

         Com_ptr<ID3D11PixelShader> shader;

         throw_if_failed(device.CreatePixelShader(bytecode.data(), bytecode.size(), nullptr,
                                                  shader.clear_and_assign()));

         entrypoint.insert(std::move(shader),
                           gsl::narrow_cast<std::uint16_t>(static_flags), flags);
      }

      group.pixel.insert(std::string{ep_name}, std::move(entrypoint));
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

void read_shader_pack(ucfb::Reader_strict<"shpk"_mn> reader,
                      ID3D11Device& device, Shader_database& database,
                      Input_layout_factory& input_layout_cache)
{
   const auto group_name =
      std::string{reader.read_child_strict<"NAME"_mn>().read_string()};

   if (database.groups.find(group_name)) return;

   Shader_group shader_group;

   auto entrypoints_reader = reader.read_child_strict<"EPTS"_mn>();

   read_entrypoints(entrypoints_reader.read_child_strict<"VSHD"_mn>(), device,
                    shader_group, input_layout_cache);
   read_entrypoints(entrypoints_reader.read_child_strict<"PSHD"_mn>(), device,
                    shader_group);

   read_rendertypes(reader.read_child_strict<"RTS_"_mn>(), shader_group,
                    database.rendertypes);

   database.groups.insert(group_name, std::move(shader_group));
}

}

auto load_shader_lvl(const std::filesystem::path lvl_path, ID3D11Device& device,
                     Input_layout_factory& input_layout_cache) noexcept -> Shader_database
{
   try {
      win32::Memeory_mapped_file file{lvl_path};
      ucfb::Reader reader{file.bytes()};
      Shader_database database;

      while (reader) {
         read_shader_pack(reader.read_child_strict<"shpk"_mn>(), device,
                          database, input_layout_cache);
      }

      return database;
   }
   catch (std::exception& e) {
      log_and_terminate("Failed to load shaders! reason: ", e.what());
   }
}

}
