
#include "munged_shader_declarations.hpp"
#include "fixedfunc_shader_metadata.hpp"
#include "shader_declarations.hpp"

#include <absl/container/flat_hash_map.h>
#include <absl/container/inlined_vector.h>

#include <algorithm>
#include <cassert>
#include <numeric>
#include <ranges>

namespace sp::game_support {

namespace {

constexpr std::size_t shader_decl_count = 23;

static_assert(shader_decl_count == std::tuple_size_v<decltype(shader_declarations)>);
static_assert(shader_decl_count ==
              std::tuple_size_v<decltype(Munged_shader_declarations::munged_declarations)>);

constexpr std::size_t variation_count = 32;

enum class Vertex_feature_flags : std::uint8_t {
   none = 0,
   skinned = 2,
   color = 8,
   texcoords = 16
};

constexpr bool marked_as_enum_flag(Vertex_feature_flags)
{
   return true;
}

enum class Pass_flags : std::uint32_t {
   none = 0,

   lighting_none = 1,
   lighting_diffuse = 2,

   transform_none = 16,
   transform_position = 32,
   transform_normal = 64,
   transform_tangents = 128,

   vertex_color = 256,
   vertex_texcoords = 512
};

constexpr bool marked_as_enum_flag(Pass_flags)
{
   return true;
}

auto add_flag_skinned_variations(absl::InlinedVector<Shader_flags, variation_count> current)
   -> absl::InlinedVector<Shader_flags, variation_count>
{
   absl::InlinedVector<Shader_flags, variation_count> variations = current;

   for (auto vari : current) variations.push_back(vari | Shader_flags::skinned);

   return variations;
}

auto add_flag_light_variations(absl::InlinedVector<Shader_flags, 32> current)
   -> absl::InlinedVector<Shader_flags, variation_count>
{
   absl::InlinedVector<Shader_flags, variation_count> variations = current;

   for (auto vari : current) {
      constexpr std::array lighting_variations{
         Shader_flags::light_dir,
         Shader_flags::light_dir | Shader_flags::light_point,
         Shader_flags::light_dir | Shader_flags::light_point2,
         Shader_flags::light_dir | Shader_flags::light_point4,
         Shader_flags::light_dir | Shader_flags::light_spot,
         Shader_flags::light_dir | Shader_flags::light_spot | Shader_flags::light_point,
         Shader_flags::light_dir | Shader_flags::light_spot | Shader_flags::light_point2};

      for (auto light_flags : lighting_variations) {
         variations.push_back(vari | light_flags);
      }
   }

   return variations;
}

auto get_flag_variations(const Shader_entry& shader) noexcept
   -> absl::InlinedVector<Shader_flags, variation_count>
{
   absl::InlinedVector<Shader_flags, variation_count> variations;

   variations.push_back(Shader_flags::none);

   if (shader.vertex_color) variations.push_back(Shader_flags::vertexcolor);

   if (shader.skinned) variations = add_flag_skinned_variations(variations);

   if (shader.lighting) variations = add_flag_light_variations(variations);

   return variations;
}

auto get_base_vertex_flags(const Shader_entry& shader) noexcept -> Vertex_shader_flags
{
   Vertex_shader_flags flags = Vertex_shader_flags::none;

   if (shader.base_input >= Base_input::position) {
      flags |= Vertex_shader_flags::position;
   }

   if (shader.base_input >= Base_input::normals) {
      flags |= Vertex_shader_flags::normal;
   }

   if (shader.base_input >= Base_input::tangents) {
      flags |= Vertex_shader_flags::tangents;
   }

   if (shader.texture_coords) {
      flags |= Vertex_shader_flags::texcoords;
   }

   return flags;
}

auto get_shader_variations(const Rendertype& rendertype, const Shader_entry& shader,
                           absl::InlinedVector<std::size_t, 1024>& local_shader_pool,
                           std::vector<Shader_metadata>& global_shader_pool) noexcept
   -> absl::InlinedVector<std::pair<Shader_flags, std::size_t>, variation_count>
{
   const Shader_metadata base_metadata{.rendertype = rendertype,
                                       .shader_name = shader.name,
                                       .vertex_shader_flags =
                                          get_base_vertex_flags(shader),
                                       .srgb_state = shader.srgb_state};

   absl::InlinedVector<std::pair<Shader_flags, std::size_t>, variation_count> variations;

   for (auto variation_flags : get_flag_variations(shader)) {
      auto metadata = base_metadata;

      if (is_flag_set(variation_flags, Shader_flags::vertexcolor)) {
         metadata.vertex_shader_flags |= Vertex_shader_flags::color;
      }

      if (is_flag_set(variation_flags, Shader_flags::skinned)) {
         metadata.vertex_shader_flags |= Vertex_shader_flags::hard_skinned;
      }

      metadata.light_active =
         is_any_flag_set(variation_flags,
                         Shader_flags::light_dir | Shader_flags::light_point2 |
                            Shader_flags::light_point4 | Shader_flags::light_spot);

      metadata.light_active_point_count = [&]() -> std::uint8_t {
         if (is_flag_set(variation_flags, Shader_flags::light_point4)) return 4;
         if (is_flag_set(variation_flags, Shader_flags::light_point2)) return 2;
         if (is_flag_set(variation_flags, Shader_flags::light_point)) return 1;

         return 0;
      }();

      metadata.light_active_spot =
         is_flag_set(variation_flags, Shader_flags::light_spot);

      const auto global_index = global_shader_pool.size();
      const auto local_index = local_shader_pool.size();

      global_shader_pool.emplace_back(std::move(metadata));
      local_shader_pool.emplace_back(global_index);
      variations.emplace_back(variation_flags, local_index);
   }

   return variations;
}

template<std::size_t pool_size>
auto get_vertex_feature_flags(const std::array<Shader_entry, pool_size>& shaders) noexcept
   -> Vertex_feature_flags
{
   auto flags = Vertex_feature_flags::none;

   if (std::ranges::any_of(shaders, [](const Shader_entry& shader) {
          return shader.skinned;
       })) {
      flags |= Vertex_feature_flags::skinned;
   }

   if (std::ranges::any_of(shaders, [](const Shader_entry& shader) {
          return shader.vertex_color;
       })) {
      flags |= Vertex_feature_flags::color;
   }

   if (std::ranges::any_of(shaders, [](const Shader_entry& shader) {
          return shader.texture_coords;
       })) {
      flags |= Vertex_feature_flags::texcoords;
   }

   return flags;
}

template<std::size_t pool_size>
auto get_pass_entry(const std::string_view shader_name,
                    const std::array<Shader_entry, pool_size>& pool)
   -> const Shader_entry&
{
   auto it = std::ranges::find_if(pool, [&](const auto& entry) noexcept {
      return entry.name == shader_name;
   });

   if (it == pool.cend()) std::terminate();

   return *it;
}

auto get_pass_flags(const Shader_entry& shader) noexcept -> Pass_flags
{
   auto flags = Pass_flags::none;

   if (!shader.lighting) {
      flags |= Pass_flags::lighting_none;
   }
   else {
      flags |= Pass_flags::lighting_diffuse;
   }

   if (shader.base_input == Base_input::none) {
      flags |= Pass_flags::transform_none;
   }
   if (shader.base_input == Base_input::position) {
      flags |= Pass_flags::transform_position;
   }
   if (shader.base_input == Base_input::normals) {
      flags |= Pass_flags::transform_normal;
   }
   if (shader.base_input == Base_input::tangents) {
      flags |= Pass_flags::transform_tangents;
   }

   if (shader.vertex_color) {
      flags |= Pass_flags::vertex_color;
   }

   if (shader.texture_coords) {
      flags |= Pass_flags::vertex_texcoords;
   }

   return flags;
}

template<std::size_t shader_pool_size, std::size_t... state_pass_counts>
auto munge_declaration(const Shader_declaration<shader_pool_size, state_pass_counts...>& declaration,
                       std::vector<Shader_metadata>& shader_pool) noexcept
   -> ucfb::Editor_parent_chunk
{
   absl::flat_hash_map<std::string_view, absl::InlinedVector<std::pair<Shader_flags, std::size_t>, variation_count>>
      shader_variation_index;

   shader_variation_index.reserve(declaration.pool.size());

   absl::InlinedVector<std::size_t, 1024> local_shader_pool;

   for (const auto& shader : declaration.pool) {
      shader_variation_index.emplace(shader.name,
                                     get_shader_variations(declaration.rendertype,
                                                           shader, local_shader_pool,
                                                           shader_pool));
   }

   ucfb::Editor_parent_chunk shdr;

   shdr.reserve(4);

   // RTYP
   {
      auto rtyp = std::get<ucfb::Editor_data_chunk>(
                     shdr.emplace_back("RTYP"_mn, ucfb::Editor_data_chunk{}).second)
                     .writer();

      rtyp.write(to_string(declaration.rendertype));
   }

   // NAME
   {
      auto name = std::get<ucfb::Editor_data_chunk>(
                     shdr.emplace_back("NAME"_mn, ucfb::Editor_data_chunk{}).second)
                     .writer();

      name.write(to_string(declaration.rendertype));
   }

   // INFO
   {
      auto info = std::get<ucfb::Editor_data_chunk>(
                     shdr.emplace_back("INFO"_mn, ucfb::Editor_data_chunk{}).second)
                     .writer();

      info.write(std::uint32_t{1}, get_vertex_feature_flags(declaration.pool));
   }

   // PIPE
   {
      std::vector<std::byte> buffer;
      {
         ucfb::Memory_writer pipe_{"PIPE"_mn, buffer};
         ucfb::Writer<ucfb::Writer_target_container<std::vector<std::byte>>>& pipe =
            pipe_; // ...sigh

         // INFO
         {
            auto info = pipe.emplace_child("INFO"_mn);

            info.write<std::uint32_t>(1);
            info.write<std::uint32_t>(std::tuple_size_v<decltype(declaration.states)>);
            info.write<std::uint32_t>(local_shader_pool.size());
            info.write<std::uint32_t>(1);
         }

         // VSP
         {
            auto vsp = pipe.emplace_child("VSP_"_mn);

            // VS
            for (const auto index : local_shader_pool) {
               auto vs = vsp.emplace_child("VS__"_mn);

               vs.write<std::uint32_t>(sizeof(std::uint32_t));
               vs.write<std::uint32_t>(index);
            }
         }

         // PSP
         {
            auto psp = pipe.emplace_child("PSP_"_mn);

            psp.emplace_child("PS__"_mn).write(sizeof(std::uint32_t), 0ul);
         }

         // STAT
         const auto write_state = [&](const auto& state, const std::size_t index) {
            auto stat = pipe.emplace_child("STAT"_mn);

            // INFO
            {
               auto info = stat.emplace_child("INFO"_mn);

               info.write<std::uint32_t>(index);
               info.write<std::uint32_t>(state.passes.size());
            }

            // PASS
            using std::views::transform;

            for (const auto& shader_name :
                 state.passes |
                    transform([](const auto& pass) { return pass.shader_name; })) {
               auto pass = stat.emplace_child("PASS"_mn);

               const auto& variations = shader_variation_index.at(shader_name);
               const auto pass_flags =
                  get_pass_flags(get_pass_entry(shader_name, declaration.pool));

               // INFO
               {
                  auto info = pass.emplace_child("INFO"_mn);

                  info.write(pass_flags);
               }

               // PVS
               for (const auto& variation : variations) {
                  auto pvs = pass.emplace_child("PVS_"_mn);

                  pvs.write(variation.first);
                  pvs.write<std::uint32_t>(variation.second);
               }

               // PPS
               pass.emplace_child("PPS_"_mn).write<std::uint32_t>(0);
            }
         };

         std::invoke(
            [&]<std::size_t... indices>(std::index_sequence<indices...>) {
               (write_state(std::get<indices>(declaration.states), indices), ...);
            },
            std::make_index_sequence<std::tuple_size_v<decltype(declaration.states)>>{});
      }

      auto& pipe = std::get<ucfb::Editor_data_chunk>(
         shdr.emplace_back("PIPE"_mn, ucfb::Editor_data_chunk{}).second);

      pipe.resize(buffer.size() - 8);

      std::ranges::copy(buffer | std::views::drop(8), pipe.begin());
   }

   return shdr;
}

auto munge_declarations() noexcept -> Munged_shader_declarations
{
   Munged_shader_declarations result;

   const auto munge_tuple = [&]<typename... Ts, std::size_t... indices>(
      const std::tuple<Ts...>& decls, std::index_sequence<indices...>) noexcept
   {
      const auto munge_tuple_decl = [&]<typename Index>(const Index) noexcept {
         constexpr auto index = Index::value;

         result.munged_declarations[index] =
            munge_declaration(std::get<index>(decls), result.shader_pool);
      };

      (munge_tuple_decl(std::integral_constant<std::size_t, indices>{}), ...);
   };

   munge_tuple(shader_declarations,
               std::make_index_sequence<std::tuple_size_v<decltype(shader_declarations)>>{});

   result.shader_pool.insert(result.shader_pool.cend(),
                             fixedfunc_shader_declarations.cbegin(),
                             fixedfunc_shader_declarations.cend());

   return result;
}

}

auto munged_shader_declarations() noexcept -> const Munged_shader_declarations&
{
   const static auto munged = munge_declarations();

   return munged;
}

}
