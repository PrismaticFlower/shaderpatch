
#include "shader_loader.hpp"
#include "../logger.hpp"
#include "memory_mapped_file.hpp"
#include "throw_if_failed.hpp"
#include "ucfb_reader.hpp"
#include "utility.hpp"

#include <string>
#include <vector>

#include <d3d11_1.h>

namespace sp::core {

using namespace std::literals;

namespace {

struct Shader_load_statistics {
   struct Stage {
      std::size_t count{};
      std::size_t created{};
      std::size_t failed{};
   };

   std::size_t group_count{};
   std::size_t rendertypes_count{};
   std::size_t state_count{};

   Stage cs;
   Stage vs;
   Stage hs;
   Stage ds;
   Stage gs;
   Stage ps;
};

auto operator+=(Shader_load_statistics::Stage& left,
                const Shader_load_statistics::Stage& right) noexcept
   -> Shader_load_statistics::Stage&
{
   left.count += right.count;
   left.created += right.created;
   left.failed += right.failed;

   return left;
}

auto operator+=(Shader_load_statistics& left, const Shader_load_statistics& right) noexcept
   -> Shader_load_statistics&
{
   left.group_count += right.group_count;
   left.rendertypes_count += right.rendertypes_count;
   left.state_count += right.state_count;

   left.cs += right.cs;
   left.vs += right.vs;
   left.hs += right.hs;
   left.ds += right.ds;
   left.gs += right.gs;
   left.ps += right.ps;

   return left;
}

auto read_input_layout(ucfb::Reader_strict<"IALO"_mn> reader)
   -> std::vector<Shader_input_element>
{
   const auto elem_count = reader.read<std::uint32_t>();

   std::vector<Shader_input_element> layout;
   layout.resize(elem_count);

   for (auto& elem : layout) {
      elem.semantic_name = reader.read_string().data();
      elem.semantic_index = reader.read<std::uint32_t>();
      elem.input_type = reader.read<Shader_input_type>();
   }

   return layout;
}

auto read_entrypoints(ucfb::Reader_strict<"CSHD"_mn> reader, ID3D11Device& device,
                      Shader_group& group) -> Shader_load_statistics::Stage
{
   const auto count = reader.read_child_strict<"SIZE"_mn>().read<std::uint32_t>();

   Shader_load_statistics::Stage stats;

   for (auto ep_index = 0u; ep_index < count; ++ep_index) {
      auto ep_reader = reader.read_child_strict<"EP__"_mn>();
      const auto ep_name = ep_reader.read_child_strict<"NAME"_mn>().read_string();

      auto vrs_reader = ep_reader.read_child_strict<"VRS_"_mn>();

      const auto variation_count =
         vrs_reader.read_child_strict<"SIZE"_mn>().read<std::uint32_t>();

      Compute_shader_entrypoint entrypoint;

      for (auto vari_index = 0u; vari_index < variation_count; ++vari_index) {
         auto vari_reader = vrs_reader.read_child_strict<"VARI"_mn>();

         const auto static_flags = vari_reader.read<std::uint32_t>();
         const auto bytecode_size = vari_reader.read<std::uint32_t>();
         const auto bytecode = vari_reader.read_array<std::byte>(bytecode_size);

         stats.count += 1;

         Com_ptr<ID3D11ComputeShader> shader;

         if (SUCCEEDED(device.CreateComputeShader(bytecode.data(), bytecode.size(), nullptr,
                                                  shader.clear_and_assign()))) {
            stats.created += 1;
         }
         else {
            stats.failed += 1;
         }

         entrypoint.insert(std::move(shader), static_flags);
      }

      group.compute.insert(std::string{ep_name}, std::move(entrypoint));
   }

   return stats;
}

auto read_entrypoints(ucfb::Reader_strict<"VSHD"_mn> reader, ID3D11Device& device,
                      Shader_group& group) -> Shader_load_statistics::Stage
{
   const auto count = reader.read_child_strict<"SIZE"_mn>().read<std::uint32_t>();

   Shader_load_statistics::Stage stats;

   for (auto ep_index = 0u; ep_index < count; ++ep_index) {
      auto ep_reader = reader.read_child_strict<"EP__"_mn>();
      const auto ep_name = ep_reader.read_child_strict<"NAME"_mn>().read_string();

      auto vrs_reader = ep_reader.read_child_strict<"VRS_"_mn>();

      const auto variation_count =
         vrs_reader.read_child_strict<"SIZE"_mn>().read<std::uint32_t>();

      Vertex_shader_entrypoint entrypoint;

      for (auto vari_index = 0u; vari_index < variation_count; ++vari_index) {
         auto vari_reader = vrs_reader.read_child_strict<"VARI"_mn>();

         const auto flags = vari_reader.read<Vertex_shader_flags>();
         const auto static_flags = vari_reader.read<std::uint32_t>();
         const auto bytecode_size = vari_reader.read<std::uint32_t>();
         const auto bytecode = vari_reader.read_array<std::byte>(bytecode_size);
         auto input_layout =
            read_input_layout(vari_reader.read_child_strict<"IALO"_mn>());

         stats.count += 1;

         Com_ptr<ID3D11VertexShader> shader;

         if (SUCCEEDED(device.CreateVertexShader(bytecode.data(), bytecode.size(), nullptr,
                                                 shader.clear_and_assign()))) {
            stats.created += 1;
         }
         else {
            stats.failed += 1;
         }

         entrypoint.insert(std::move(shader), make_vector(bytecode),
                           std::move(input_layout),
                           gsl::narrow_cast<std::uint16_t>(static_flags), flags);
      }

      group.vertex.insert(std::string{ep_name}, std::move(entrypoint));
   }

   return stats;
}

auto read_entrypoints(ucfb::Reader_strict<"HSHD"_mn> reader, ID3D11Device& device,
                      Shader_group& group) -> Shader_load_statistics::Stage
{
   const auto count = reader.read_child_strict<"SIZE"_mn>().read<std::uint32_t>();

   Shader_load_statistics::Stage stats;

   for (auto ep_index = 0u; ep_index < count; ++ep_index) {
      auto ep_reader = reader.read_child_strict<"EP__"_mn>();
      const auto ep_name = ep_reader.read_child_strict<"NAME"_mn>().read_string();

      auto vrs_reader = ep_reader.read_child_strict<"VRS_"_mn>();

      const auto variation_count =
         vrs_reader.read_child_strict<"SIZE"_mn>().read<std::uint32_t>();

      Hull_shader_entrypoint entrypoint;

      for (auto vari_index = 0u; vari_index < variation_count; ++vari_index) {
         auto vari_reader = vrs_reader.read_child_strict<"VARI"_mn>();

         const auto static_flags = vari_reader.read<std::uint32_t>();
         const auto bytecode_size = vari_reader.read<std::uint32_t>();
         const auto bytecode = vari_reader.read_array<std::byte>(bytecode_size);

         stats.count += 1;

         Com_ptr<ID3D11HullShader> shader;

         if (SUCCEEDED(device.CreateHullShader(bytecode.data(), bytecode.size(),
                                               nullptr, shader.clear_and_assign()))) {
            stats.created += 1;
         }
         else {
            stats.failed += 1;
         }

         entrypoint.insert(std::move(shader), static_flags);
      }

      group.hull.insert(std::string{ep_name}, std::move(entrypoint));
   }

   return stats;
}

auto read_entrypoints(ucfb::Reader_strict<"DSHD"_mn> reader, ID3D11Device& device,
                      Shader_group& group) -> Shader_load_statistics::Stage
{
   const auto count = reader.read_child_strict<"SIZE"_mn>().read<std::uint32_t>();

   Shader_load_statistics::Stage stats;

   for (auto ep_index = 0u; ep_index < count; ++ep_index) {
      auto ep_reader = reader.read_child_strict<"EP__"_mn>();
      const auto ep_name = ep_reader.read_child_strict<"NAME"_mn>().read_string();

      auto vrs_reader = ep_reader.read_child_strict<"VRS_"_mn>();

      const auto variation_count =
         vrs_reader.read_child_strict<"SIZE"_mn>().read<std::uint32_t>();

      Domain_shader_entrypoint entrypoint;

      for (auto vari_index = 0u; vari_index < variation_count; ++vari_index) {
         auto vari_reader = vrs_reader.read_child_strict<"VARI"_mn>();

         const auto static_flags = vari_reader.read<std::uint32_t>();
         const auto bytecode_size = vari_reader.read<std::uint32_t>();
         const auto bytecode = vari_reader.read_array<std::byte>(bytecode_size);

         stats.count += 1;

         Com_ptr<ID3D11DomainShader> shader;

         if (SUCCEEDED(device.CreateDomainShader(bytecode.data(), bytecode.size(), nullptr,
                                                 shader.clear_and_assign()))) {
            stats.created += 1;
         }
         else {
            stats.failed += 1;
         }

         entrypoint.insert(std::move(shader), static_flags);
      }

      group.domain.insert(std::string{ep_name}, std::move(entrypoint));
   }

   return stats;
}

auto read_entrypoints(ucfb::Reader_strict<"GSHD"_mn> reader, ID3D11Device& device,
                      Shader_group& group) -> Shader_load_statistics::Stage
{
   const auto count = reader.read_child_strict<"SIZE"_mn>().read<std::uint32_t>();

   Shader_load_statistics::Stage stats;

   for (auto ep_index = 0u; ep_index < count; ++ep_index) {
      auto ep_reader = reader.read_child_strict<"EP__"_mn>();
      const auto ep_name = ep_reader.read_child_strict<"NAME"_mn>().read_string();

      auto vrs_reader = ep_reader.read_child_strict<"VRS_"_mn>();

      const auto variation_count =
         vrs_reader.read_child_strict<"SIZE"_mn>().read<std::uint32_t>();

      Geometry_shader_entrypoint entrypoint;

      for (auto vari_index = 0u; vari_index < variation_count; ++vari_index) {
         auto vari_reader = vrs_reader.read_child_strict<"VARI"_mn>();

         const auto static_flags = vari_reader.read<std::uint32_t>();
         const auto bytecode_size = vari_reader.read<std::uint32_t>();
         const auto bytecode = vari_reader.read_array<std::byte>(bytecode_size);

         stats.count += 1;

         Com_ptr<ID3D11GeometryShader> shader;

         if (SUCCEEDED(device.CreateGeometryShader(bytecode.data(), bytecode.size(), nullptr,
                                                   shader.clear_and_assign()))) {
            stats.created += 1;
         }
         else {
            stats.failed += 1;
         }

         entrypoint.insert(std::move(shader), static_flags);
      }

      group.geometry.insert(std::string{ep_name}, std::move(entrypoint));
   }

   return stats;
}

auto read_entrypoints(ucfb::Reader_strict<"PSHD"_mn> reader, ID3D11Device& device,
                      Shader_group& group) -> Shader_load_statistics::Stage
{
   const auto count = reader.read_child_strict<"SIZE"_mn>().read<std::uint32_t>();

   Shader_load_statistics::Stage stats;

   for (auto ep_index = 0u; ep_index < count; ++ep_index) {
      auto ep_reader = reader.read_child_strict<"EP__"_mn>();
      const auto ep_name = ep_reader.read_child_strict<"NAME"_mn>().read_string();

      auto vrs_reader = ep_reader.read_child_strict<"VRS_"_mn>();

      const auto variation_count =
         vrs_reader.read_child_strict<"SIZE"_mn>().read<std::uint32_t>();

      Pixel_shader_entrypoint entrypoint;

      for (auto vari_index = 0u; vari_index < variation_count; ++vari_index) {
         auto vari_reader = vrs_reader.read_child_strict<"VARI"_mn>();

         const auto static_flags = vari_reader.read<std::uint32_t>();
         const auto bytecode_size = vari_reader.read<std::uint32_t>();
         const auto bytecode = vari_reader.read_array<std::byte>(bytecode_size);

         stats.count += 1;

         Com_ptr<ID3D11PixelShader> shader;

         if (SUCCEEDED(device.CreatePixelShader(bytecode.data(), bytecode.size(), nullptr,
                                                shader.clear_and_assign()))) {
            stats.created += 1;
         }
         else {
            stats.failed += 1;
         }

         entrypoint.insert(std::move(shader),
                           gsl::narrow_cast<std::uint16_t>(static_flags));
      }

      group.pixel.insert(std::string{ep_name}, std::move(entrypoint));
   }

   return stats;
}

void read_state(ucfb::Reader_strict<"STAT"_mn> reader,
                const Shader_group& shader_group, Shader_rendertype& rendertype)
{
   const auto state_name =
      std::string{reader.read_child_strict<"NAME"_mn>().read_string()};

   auto info_reader = reader.read_child_strict<"INFO"_mn>();

   const auto vs_entrypoint = std::string{info_reader.read_string()};
   const auto vs_static_flags =
      gsl::narrow_cast<std::uint16_t>(info_reader.read<std::uint32_t>());
   const auto ps_entrypoint = std::string{info_reader.read_string()};
   const auto ps_static_flags =
      gsl::narrow_cast<std::uint16_t>(info_reader.read<std::uint32_t>());
   const auto ps_oit_entrypoint = std::string{info_reader.read_string()};
   const auto ps_oit_static_flags =
      gsl::narrow_cast<std::uint16_t>(info_reader.read<std::uint32_t>());

   Shader_state state;

   state.vertex = Shader_state_stage_vertex{Vertex_shader_entrypoint{
                                               shader_group.vertex.at(vs_entrypoint)},
                                            vs_static_flags};
   state.pixel = shader_group.pixel.at(ps_entrypoint).copy(ps_static_flags);
   state.pixel_oit =
      shader_group.pixel.at(ps_oit_entrypoint).copy_if(ps_oit_static_flags);

   const auto hs_use = info_reader.read<bool>();
   const auto hs_entrypoint = std::string{info_reader.read_string()};
   const auto hs_static_flags =
      gsl::narrow_cast<std::uint16_t>(info_reader.read<std::uint32_t>());
   const auto ds_use = info_reader.read<bool>();
   const auto ds_entrypoint = std::string{info_reader.read_string()};
   const auto ds_static_flags =
      gsl::narrow_cast<std::uint16_t>(info_reader.read<std::uint32_t>());
   const auto gs_use = info_reader.read<bool>();
   const auto gs_entrypoint = std::string{info_reader.read_string()};
   const auto gs_static_flags =
      gsl::narrow_cast<std::uint16_t>(info_reader.read<std::uint32_t>());

   if (hs_use)
      state.hull = shader_group.hull.at(hs_entrypoint).copy(hs_static_flags);

   if (ds_use)
      state.domain = shader_group.domain.at(ds_entrypoint).copy(ds_static_flags);

   if (gs_use)
      state.geometry = shader_group.geometry.at(gs_entrypoint).copy(gs_static_flags);

   rendertype.insert(state_name, std::move(state));
}

auto read_rendertypes(ucfb::Reader_strict<"RTS_"_mn> reader,
                      const Shader_group& shader_group,
                      Shader_rendertype_collection& rendertype_collection)
   -> Shader_load_statistics
{
   const auto count = reader.read_child_strict<"SIZE"_mn>().read<std::uint32_t>();

   Shader_load_statistics stats;

   for (auto rt_index = 0u; rt_index < count; ++rt_index) {
      auto rtyp_reader = reader.read_child_strict<"RTYP"_mn>();

      stats.rendertypes_count += 1;

      const auto rendertype_name =
         std::string{rtyp_reader.read_child_strict<"NAME"_mn>().read_string()};
      const auto state_count =
         rtyp_reader.read_child_strict<"SIZE"_mn>().read<std::uint32_t>();

      stats.state_count += state_count;

      Shader_rendertype rendertype;

      for (auto state_index = 0u; state_index < state_count; ++state_index) {
         read_state(rtyp_reader.read_child_strict<"STAT"_mn>(), shader_group, rendertype);
      }

      rendertype_collection.insert(rendertype_name, std::move(rendertype));
   }

   return stats;
}

auto read_shader_pack(ucfb::Reader_strict<"shpk"_mn> reader, ID3D11Device& device,
                      Shader_database& database) -> Shader_load_statistics
{
   const auto group_name =
      std::string{reader.read_child_strict<"NAME"_mn>().read_string()};

   if (database.groups.find(group_name)) return {};

   Shader_load_statistics stats{};

   Shader_group shader_group;

   auto entrypoints_reader = reader.read_child_strict<"EPTS"_mn>();

   stats.cs = read_entrypoints(entrypoints_reader.read_child_strict<"CSHD"_mn>(),
                               device, shader_group);
   stats.vs = read_entrypoints(entrypoints_reader.read_child_strict<"VSHD"_mn>(),
                               device, shader_group);
   stats.hs = read_entrypoints(entrypoints_reader.read_child_strict<"HSHD"_mn>(),
                               device, shader_group);
   stats.ds = read_entrypoints(entrypoints_reader.read_child_strict<"DSHD"_mn>(),
                               device, shader_group);
   stats.gs = read_entrypoints(entrypoints_reader.read_child_strict<"GSHD"_mn>(),
                               device, shader_group);
   stats.ps = read_entrypoints(entrypoints_reader.read_child_strict<"PSHD"_mn>(),
                               device, shader_group);

   stats += read_rendertypes(reader.read_child_strict<"RTS_"_mn>(),
                             shader_group, database.rendertypes);

   database.groups.insert(group_name, std::move(shader_group));

   return stats;
}
}

auto load_shader_lvl(const std::filesystem::path lvl_path, ID3D11Device& device) noexcept
   -> Shader_database
{
   try {
      win32::Memeory_mapped_file file{lvl_path};
      ucfb::Reader reader{file.bytes()};
      Shader_database database;

      Shader_load_statistics stats{};

      while (reader) {
         stats += read_shader_pack(reader.read_child_strict<"shpk"_mn>(),
                                   device, database);
         stats.group_count += 1;
      }

      log(Log_level::info, "Shaders Loaded.", " group_count: ", stats.group_count,
          " rendertypes_count: ", stats.rendertypes_count,
          " state_count: ", stats.state_count, " cs.count: ", stats.cs.count,
          " cs.created: ", stats.cs.created, " cs.failed: ", stats.cs.failed,
          " vs.count: ", stats.vs.count, " vs.created: ", stats.vs.created,
          " vs.failed: ", stats.vs.failed, " hs.count: ", stats.hs.count,
          " hs.created: ", stats.hs.created, " hs.failed: ", stats.hs.failed,
          " ds.count: ", stats.ds.count, " ds.created: ", stats.ds.created,
          " ds.failed: ", stats.ds.failed, " gs.count: ", stats.gs.count,
          " gs.created: ", stats.gs.created, " gs.failed: ", stats.gs.failed,
          " ps.count: ", stats.ps.count, " ps.created: ", stats.ps.created,
          " ps.failed: ", stats.ps.failed);

      return database;
   }
   catch (std::exception& e) {
      log_and_terminate("Failed to load shaders! reason: ", e.what());
   }
}
}
