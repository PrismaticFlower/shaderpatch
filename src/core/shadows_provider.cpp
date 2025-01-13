
#include "shadows_provider.hpp"
#include "../effects/profiler.hpp"
#include "../logger.hpp"
#include "../shadows/shadow_world.hpp"
#include "d3d11_helpers.hpp"
#include "shadows_camera_helpers.hpp"

#include <ranges>

#include <absl/container/flat_hash_set.h>

using namespace std::literals;
using sp::shader::Vertex_shader_flags;

#include "../imgui/imgui.h"

namespace sp::core {

using effects::Profile;

namespace {

struct alignas(256) Camera_cb {
   std::array<glm::mat4, 4> view_proj_matrices;
};

static_assert(sizeof(Camera_cb) == 256);

struct alignas(256) Transform_cb {
   glm::vec3 position_decompress_min;
   std::uint32_t skin_index;
   glm::vec3 position_decompress_max;
   std::uint32_t padding;
   std::array<glm::vec4, 3> world_matrix;
   glm::vec4 x_texcoord_transform;
   glm::vec4 y_texcoord_transform;
};

static_assert(sizeof(Transform_cb) == 256);

struct alignas(16) Skin_cb {
   std::array<std::array<glm::vec4, 3>, 15> bone_matrices;
};

struct alignas(256) Draw_to_target_cb {
   glm::mat4 inv_view_proj_matrix;
   std::array<glm::mat4, 4> shadow_matrices;
   glm::vec2 target_resolution;
   float shadow_bias;
};

static_assert(sizeof(Draw_to_target_cb) == 512);

constexpr UINT cascade_count = 4;
constexpr UINT TEMP_shadow_map_length = 2048;

// clang-format off


template<typename T>
concept Hardedged_mesh = requires(T mesh)
{
   { mesh.x_texcoord_transform } -> std::convertible_to<glm::vec4>;
   { mesh.y_texcoord_transform } -> std::convertible_to<glm::vec4>;
};

// clang-format on

auto make_input_layout(
   ID3D11Device5& device, std::initializer_list<D3D11_INPUT_ELEMENT_DESC> input_layout,
   std::tuple<Com_ptr<ID3D11VertexShader>, shader::Bytecode_blob, shader::Vertex_input_layout> shader) noexcept
   -> Com_ptr<ID3D11InputLayout>
{
   auto& [_0, bytecode, _1] = shader;

   Com_ptr<ID3D11InputLayout> result;

   if (FAILED(device.CreateInputLayout(input_layout.begin(), input_layout.size(),
                                       bytecode.data(), bytecode.size(),
                                       result.clear_and_assign()))) {
      log_and_terminate("Failed to create shadow mesh input layout!");
   }

   return result;
}

auto make_bounding_box(const auto& mesh) noexcept -> shadows::Bounding_box
{
   glm::vec3 min =
      glm::vec3{static_cast<float>(-std::numeric_limits<std::int16_t>::max())}; // negated max is not a mistake
   glm::vec3 max =
      glm::vec3{static_cast<float>(std::numeric_limits<std::int16_t>::max())};

   min = mesh.position_decompress_max + (mesh.position_decompress_min * min);
   max = mesh.position_decompress_max + (mesh.position_decompress_min * max);

   const std::array<glm::vec4, 8> vertices{{{min.x, min.y, min.z, 1.0f},
                                            {max.x, min.y, min.z, 1.0f},
                                            {min.x, max.y, min.z, 1.0f},
                                            {max.x, max.y, min.z, 1.0f},
                                            {min.x, min.y, max.z, 1.0f},
                                            {max.x, min.y, max.z, 1.0f},
                                            {min.x, max.y, max.z, 1.0f},
                                            {max.x, max.y, max.z, 1.0f}}};

   min = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
          std::numeric_limits<float>::max()};
   max = {std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(),
          std::numeric_limits<float>::lowest()};

   for (const auto& v : vertices) {
      glm::vec3 posWS = {glm::dot(v, mesh.world_matrix[0]),
                         glm::dot(v, mesh.world_matrix[1]),
                         glm::dot(v, mesh.world_matrix[2])};

      min = glm::min(min, posWS);
      max = glm::max(max, posWS);
   }

   return {.min = min, .max = max};
}

auto make_bounding_sphere(const auto& mesh) noexcept -> glm::vec4
{
   glm::vec3 min =
      glm::vec3{static_cast<float>(std::numeric_limits<std::int16_t>::min())};
   glm::vec3 max =
      glm::vec3{static_cast<float>(std::numeric_limits<std::int16_t>::max())};

   min = mesh.position_decompress_max + (mesh.position_decompress_min * min);
   max = mesh.position_decompress_max + (mesh.position_decompress_min * max);

   glm::vec3 centre = (min + max) / 2.0f;

   return {centre + glm::vec3{mesh.world_matrix[0][3], mesh.world_matrix[1][3],
                              mesh.world_matrix[2][3]},
           glm::distance(min, max) * 0.5f};
}

}

Shadows_provider::Shadows_provider(Com_ptr<ID3D11Device5> device,
                                   shader::Database& shaders) noexcept
   : _device{device},
     _mesh_hardedged{shaders.vertex("shadowmesh_zprepass"sv)
                        .entrypoint("hardedged_vs"sv, 0, Vertex_shader_flags::none)},
     _mesh_hardedged_compressed{
        shaders.vertex("shadowmesh_zprepass"sv)
           .entrypoint("hardedged_vs"sv, 0, Vertex_shader_flags::compressed)},
     _mesh_hardedged_skinned{
        shaders.vertex("shadowmesh_zprepass"sv)
           .entrypoint("hardedged_vs"sv, 0, Vertex_shader_flags::hard_skinned)},
     _mesh_hardedged_compressed_skinned{
        shaders.vertex("shadowmesh_zprepass"sv)
           .entrypoint("hardedged_vs"sv, 0,
                       Vertex_shader_flags::compressed | Vertex_shader_flags::hard_skinned)}
{
   resize_transform_cb_buffer(768);
   resize_skins_buffer(64);
   create_shadow_map(TEMP_shadow_map_length);

   auto& shader = shaders.vertex("shadowmesh_zprepass"sv);

   _mesh_il = make_input_layout(*device,
                                {{.SemanticName = "POSITION",
                                  .Format = DXGI_FORMAT_R32G32B32_FLOAT}},
                                shader.entrypoint("opaque_vs"sv, 0,
                                                  Vertex_shader_flags::none));
   _mesh_compressed_il =
      make_input_layout(*device,
                        {{.SemanticName = "POSITION",
                          .Format = DXGI_FORMAT_R16G16B16A16_SINT}},
                        shader.entrypoint("opaque_vs"sv, 0,
                                          Vertex_shader_flags::compressed));
   _mesh_skinned_il =
      make_input_layout(*device,
                        {{.SemanticName = "POSITION", .Format = DXGI_FORMAT_R32G32B32_FLOAT},
                         {.SemanticName = "BLENDINDICES",
                          .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                          .AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT}},
                        shader.entrypoint("opaque_vs"sv, 0,
                                          Vertex_shader_flags::hard_skinned));
   _mesh_compressed_skinned_il =
      make_input_layout(*device,
                        {{.SemanticName = "POSITION", .Format = DXGI_FORMAT_R16G16B16A16_SINT},
                         {.SemanticName = "BLENDINDICES",
                          .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                          .AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT}},
                        shader.entrypoint("opaque_vs"sv, 0,
                                          Vertex_shader_flags::compressed |
                                             Vertex_shader_flags::hard_skinned));
   _mesh_stencilshadow_skinned_il =
      make_input_layout(*device,
                        {{.SemanticName = "POSITION", .Format = DXGI_FORMAT_R16G16B16A16_SINT},
                         {.SemanticName = "BLENDINDICES",
                          .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                          .AlignedByteOffset = 12}},
                        shader.entrypoint("opaque_vs"sv, 0,
                                          Vertex_shader_flags::compressed |
                                             Vertex_shader_flags::hard_skinned));

   _mesh_vs = std::get<Com_ptr<ID3D11VertexShader>>(
      shader.entrypoint("opaque_vs"sv, 0, Vertex_shader_flags::none));
   _mesh_compressed_vs = std::get<Com_ptr<ID3D11VertexShader>>(
      shader.entrypoint("opaque_vs"sv, 0, Vertex_shader_flags::compressed));
   _mesh_skinned_vs = std::get<Com_ptr<ID3D11VertexShader>>(
      shader.entrypoint("opaque_vs"sv, 0, Vertex_shader_flags::hard_skinned));
   _mesh_compressed_skinned_vs = std::get<Com_ptr<ID3D11VertexShader>>(
      shader.entrypoint("opaque_vs"sv, 0,
                        Vertex_shader_flags::compressed |
                           Vertex_shader_flags::hard_skinned));

   _mesh_hardedged_ps =
      shaders.pixel("shadowmesh_zprepass"sv).entrypoint("hardedged_ps"sv);

   _draw_to_target_vs = std::get<Com_ptr<ID3D11VertexShader>>(
      shaders.vertex("shadowmap render"sv).entrypoint("main_vs"sv, 0, Vertex_shader_flags::none));

   _draw_to_target_ps = shaders.pixel("shadowmap render"sv).entrypoint("main_ps"sv);

   _camera_cb_buffer = [&] {
      Com_ptr<ID3D11Buffer> buffer;

      const D3D11_BUFFER_DESC desc{.ByteWidth = sizeof(Camera_cb),
                                   .Usage = D3D11_USAGE_DYNAMIC,
                                   .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
                                   .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE};

      if (FAILED(device->CreateBuffer(&desc, nullptr, buffer.clear_and_assign()))) {
         log_and_terminate("Unable to create shadow camera buffer!");
      }

      return buffer;
   }();

   _draw_to_target_cb = [&] {
      Com_ptr<ID3D11Buffer> buffer;

      const D3D11_BUFFER_DESC desc{.ByteWidth = sizeof(Draw_to_target_cb),
                                   .Usage = D3D11_USAGE_DYNAMIC,
                                   .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
                                   .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE};

      if (FAILED(device->CreateBuffer(&desc, nullptr, buffer.clear_and_assign()))) {
         log_and_terminate("Unable to create shadow draw to target buffer!");
      }

      return buffer;
   }();

   _rasterizer_state = [&] {
      Com_ptr<ID3D11RasterizerState> rasterizer_state;

      const D3D11_RASTERIZER_DESC desc{.FillMode = D3D11_FILL_SOLID,
                                       .CullMode = D3D11_CULL_BACK,
                                       .FrontCounterClockwise = true};

      if (FAILED(device->CreateRasterizerState(&desc,
                                               rasterizer_state.clear_and_assign()))) {
         log_and_terminate("Unable to create shadow rasterizer state!");
      }

      return rasterizer_state;
   }();

   _rasterizer_doublesided_state = [&] {
      Com_ptr<ID3D11RasterizerState> rasterizer_state;

      const D3D11_RASTERIZER_DESC desc{.FillMode = D3D11_FILL_SOLID,
                                       .CullMode = D3D11_CULL_NONE,
                                       .FrontCounterClockwise = true};

      if (FAILED(device->CreateRasterizerState(&desc,
                                               rasterizer_state.clear_and_assign()))) {
         log_and_terminate("Unable to create shadow rasterizer state!");
      }

      return rasterizer_state;
   }();

   _draw_to_target_depthstencil_state = [&] {
      Com_ptr<ID3D11DepthStencilState> state;

      const D3D11_DEPTH_STENCIL_DESC desc{.DepthEnable = true,
                                          .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO,
                                          .DepthFunc = D3D11_COMPARISON_NOT_EQUAL,
                                          .StencilEnable = false};

      if (FAILED(device->CreateDepthStencilState(&desc, state.clear_and_assign()))) {
         log_and_terminate("Unable to create shadow depthstencil state!");
      }

      return state;
   }();

   _shadow_map_sampler = [&] {
      Com_ptr<ID3D11SamplerState> state;

      const D3D11_SAMPLER_DESC desc{.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
                                    .AddressU = D3D11_TEXTURE_ADDRESS_BORDER,
                                    .AddressV = D3D11_TEXTURE_ADDRESS_BORDER,
                                    .AddressW = D3D11_TEXTURE_ADDRESS_BORDER,
                                    .MipLODBias = 0.0f,
                                    .MaxAnisotropy = 1,
                                    .ComparisonFunc = D3D11_COMPARISON_LESS,
                                    .BorderColor = {0.0f, 0.0f, 0.0f, 0.0f},
                                    .MinLOD = 0.0f,
                                    .MaxLOD = D3D11_FLOAT32_MAX};

      if (FAILED(device->CreateSamplerState(&desc, state.clear_and_assign()))) {
         log_and_terminate("Unable to create shadow sampler state!");
      }

      return state;
   }();

   _hardedged_texture_sampler = [&] {
      Com_ptr<ID3D11SamplerState> state;

      const D3D11_SAMPLER_DESC desc{.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
                                    .AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
                                    .AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
                                    .AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
                                    .MipLODBias = 0.0f,
                                    .MaxAnisotropy = 1,
                                    .ComparisonFunc = D3D11_COMPARISON_ALWAYS,
                                    .BorderColor = {0.0f, 0.0f, 0.0f, 0.0f},
                                    .MinLOD = 0.0f,
                                    .MaxLOD = D3D11_FLOAT32_MAX};

      if (FAILED(device->CreateSamplerState(&desc, state.clear_and_assign()))) {
         log_and_terminate("Unable to create shadow sampler state!");
      }

      return state;
   }();
}

void Shadows_provider::end_frame() noexcept
{
   meshes.clear();
}

void Shadows_provider::draw_shadow_maps(ID3D11DeviceContext4& dc,
                                        const Draw_args& args) noexcept
{
   prepare_draw_shadow_maps(dc, args);

   dc.ClearState();

   draw_shadow_maps_instanced(dc, args.input_layout_descriptions, args.profiler);

   const D3D11_VIEWPORT cascade_viewport = {
      .Width = _shadow_map_length_flt,
      .Height = _shadow_map_length_flt,
      .MinDepth = 0.0f,
      .MaxDepth = 1.0f,
   };

   const std::array<shadows::Shadow_draw_view, 4> shadow_views = {
      shadows::Shadow_draw_view{
         .viewport = cascade_viewport,
         .dsv = _shadow_map_dsvs[0].get(),
         .shadow_projection_matrix = _cascade_view_proj_matrices[0],
      },

      shadows::Shadow_draw_view{
         .viewport = cascade_viewport,
         .dsv = _shadow_map_dsvs[1].get(),
         .shadow_projection_matrix = _cascade_view_proj_matrices[1],
      },

      shadows::Shadow_draw_view{
         .viewport = cascade_viewport,
         .dsv = _shadow_map_dsvs[2].get(),
         .shadow_projection_matrix = _cascade_view_proj_matrices[2],
      },

      shadows::Shadow_draw_view{
         .viewport = cascade_viewport,
         .dsv = _shadow_map_dsvs[3].get(),
         .shadow_projection_matrix = _cascade_view_proj_matrices[3],
      },
   };

   shadows::shadow_world.draw_shadow_views(dc, view_proj_matrix, shadow_views);

   draw_to_target_map(dc, args);

   dc.DiscardResource(_shadow_map_texture.get());
}

void Shadows_provider::prepare_draw_shadow_maps(ID3D11DeviceContext4& dc,
                                                const Draw_args& args) noexcept
{
   build_cascade_info();

   upload_buffer_data(dc, args);
}

void Shadows_provider::build_cascade_info() noexcept
{
   auto cascades = make_shadow_cascades(glm::normalize(-light_direction),
                                        view_proj_matrix, config.start_depth,
                                        config.end_depth, _shadow_map_length_flt);

   _cascade_view_proj_matrices =
      {glm::transpose(cascades[0].view_projection_matrix()),
       glm::transpose(cascades[1].view_projection_matrix()),
       glm::transpose(cascades[2].view_projection_matrix()),
       glm::transpose(cascades[3].view_projection_matrix())};

   _cascade_texture_matrices = {glm::transpose(cascades[0].texture_matrix()),
                                glm::transpose(cascades[1].texture_matrix()),
                                glm::transpose(cascades[2].texture_matrix()),
                                glm::transpose(cascades[3].texture_matrix())};

   // TODO: Frustum culling.
}

void Shadows_provider::upload_buffer_data(ID3D11DeviceContext4& dc,
                                          const Draw_args& args) noexcept
{
   upload_camera_cb_buffer(dc);
   upload_transform_cb_buffer(dc);
   upload_skins_buffer(dc);
   upload_draw_to_target_buffer(dc, args);
}

void Shadows_provider::upload_camera_cb_buffer(ID3D11DeviceContext4& dc) noexcept
{
   const Camera_cb cb{.view_proj_matrices = _cascade_view_proj_matrices};

   update_dynamic_buffer(dc, *_camera_cb_buffer, cb);
}

void Shadows_provider::upload_transform_cb_buffer(ID3D11DeviceContext4& dc) noexcept
{
   const std::size_t transforms_count = count_active_meshes();

   if (transforms_count == 0) return;

   if (transforms_count > _transforms_cb_space) {
      resize_transform_cb_buffer(transforms_count);
   }

   D3D11_MAPPED_SUBRESOURCE mapped{};

   if (FAILED(dc.Map(_transforms_cb_buffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0,
                     &mapped))) {
      log_and_terminate("Failed to map shadows transforms buffer!");
   }

   std::byte* const mapped_data = static_cast<std::byte*>(mapped.pData);
   std::size_t transform_offset = 0;

   const auto process_meshes = [&]<std::ranges::forward_range T>(const T& meshes) {
      std::size_t i = 0;

      for (const auto& mesh : meshes) {
         Transform_cb transform{.position_decompress_min = mesh.position_decompress_min,
                                .skin_index = mesh.skin_index,
                                .position_decompress_max = mesh.position_decompress_max,
                                .world_matrix = mesh.world_matrix};

         if constexpr (Hardedged_mesh<std::ranges::range_value_t<T>>) {
            transform.x_texcoord_transform = mesh.x_texcoord_transform;
            transform.y_texcoord_transform = mesh.y_texcoord_transform;
         }

         std::memcpy(mapped_data + (transform_offset + i) * sizeof(Transform_cb),
                     &transform, sizeof(Transform_cb));

         i += 1;
      }

      transform_offset += meshes.size();
   };

   process_meshes(meshes.zprepass);
   process_meshes(meshes.zprepass_compressed);
   process_meshes(meshes.zprepass_skinned);
   process_meshes(meshes.zprepass_compressed_skinned);
   process_meshes(meshes.zprepass_hardedged);
   process_meshes(meshes.zprepass_hardedged_compressed);
   process_meshes(meshes.zprepass_hardedged_skinned);
   process_meshes(meshes.zprepass_hardedged_compressed_skinned);

   dc.Unmap(_transforms_cb_buffer.get(), 0);
}

void Shadows_provider::upload_skins_buffer(ID3D11DeviceContext4& dc) noexcept
{
   if (meshes.skins.size() > _skins_space) {
      resize_skins_buffer(meshes.skins.size());
   }

   D3D11_MAPPED_SUBRESOURCE mapped{};

   if (FAILED(dc.Map(_skins_buffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
      log_and_terminate("Failed to map shadows skins buffer!");
   }

   std::byte* const mapped_data = static_cast<std::byte*>(mapped.pData);

   for (std::size_t i = 0; i < meshes.skins.size(); ++i) {
      static_assert(sizeof(Meshes::Skin) == sizeof(Skin_cb));

      std::memcpy(mapped_data + i * sizeof(Skin_cb), &meshes.skins[i], sizeof(Skin_cb));
   }

   dc.Unmap(_skins_buffer.get(), 0);
}

void Shadows_provider::upload_draw_to_target_buffer(ID3D11DeviceContext4& dc,
                                                    const Draw_args& args) noexcept
{
   const Draw_to_target_cb cb{.inv_view_proj_matrix =
                                 glm::mat4{glm::inverse(glm::dmat4{view_proj_matrix})},
                              .shadow_matrices = _cascade_texture_matrices,
                              .target_resolution =
                                 glm::vec2{args.target_width, args.target_height},
                              .shadow_bias = config.shadow_bias};

   update_dynamic_buffer(dc, *_draw_to_target_cb, cb);
}

void Shadows_provider::resize_transform_cb_buffer(std::size_t needed_space) noexcept
{
   _transforms_cb_space = needed_space + (needed_space / 2);

   const D3D11_BUFFER_DESC desc{.ByteWidth = _transforms_cb_space * sizeof(Transform_cb),
                                .Usage = D3D11_USAGE_DYNAMIC,
                                .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
                                .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE};

   if (FAILED(_device->CreateBuffer(&desc, nullptr,
                                    _transforms_cb_buffer.clear_and_assign()))) {
      log_and_terminate("Unable to create shadow transforms buffer!");
   }
}

void Shadows_provider::resize_skins_buffer(std::size_t needed_space) noexcept
{
   _skins_space = needed_space + (needed_space / 2);

   const D3D11_BUFFER_DESC desc{.ByteWidth = _skins_space * sizeof(Skin_cb),
                                .Usage = D3D11_USAGE_DYNAMIC,
                                .BindFlags = D3D11_BIND_SHADER_RESOURCE,
                                .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
                                .MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
                                .StructureByteStride = sizeof(Skin_cb)};

   if (FAILED(_device->CreateBuffer(&desc, nullptr, _skins_buffer.clear_and_assign()))) {
      log_and_terminate("Unable to create shadow skins buffer!");
   }

   if (FAILED(_device->CreateShaderResourceView(_skins_buffer.get(), nullptr,
                                                _skins_srv.clear_and_assign()))) {
      log_and_terminate("Unable to create shadow skins SRV!");
   }
}

auto Shadows_provider::count_active_meshes() const noexcept -> std::size_t
{
   return meshes.zprepass.size() + meshes.zprepass_compressed.size() +
          meshes.zprepass_skinned.size() + meshes.zprepass_compressed_skinned.size() +
          meshes.zprepass_hardedged.size() +
          meshes.zprepass_hardedged_compressed.size() +
          meshes.zprepass_hardedged_skinned.size() +
          meshes.zprepass_hardedged_compressed_skinned.size();
}

void Shadows_provider::create_shadow_map(const UINT length) noexcept
{
   const D3D11_TEXTURE2D_DESC desc{.Width = length,
                                   .Height = length,
                                   .MipLevels = 1,
                                   .ArraySize = cascade_count,
                                   .Format = DXGI_FORMAT_R32_TYPELESS,
                                   .SampleDesc = {1, 0},
                                   .Usage = D3D11_USAGE_DEFAULT,
                                   .BindFlags = D3D11_BIND_DEPTH_STENCIL |
                                                D3D11_BIND_SHADER_RESOURCE};

   if (FAILED(_device->CreateTexture2D(&desc, nullptr,
                                       _shadow_map_texture.clear_and_assign()))) {
      log_and_terminate("Unable to create shadow map!");
   }

   {
      const D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc{
         .Format = DXGI_FORMAT_D32_FLOAT,
         .ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY,
         .Texture2DArray = {.MipSlice = 0, .FirstArraySlice = 0, .ArraySize = cascade_count}};

      if (FAILED(_device->CreateDepthStencilView(_shadow_map_texture.get(), &dsv_desc,
                                                 _shadow_map_dsv.clear_and_assign()))) {
         log_and_terminate("Unable to create shadow map DSV!");
      }
   }

   for (std::size_t i = 0; i < _shadow_map_dsvs.size(); ++i) {
      const D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc{
         .Format = DXGI_FORMAT_D32_FLOAT,
         .ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY,
         .Texture2DArray = {.MipSlice = 0, .FirstArraySlice = i, .ArraySize = 1}};

      if (FAILED(_device->CreateDepthStencilView(_shadow_map_texture.get(), &dsv_desc,
                                                 _shadow_map_dsvs[i].clear_and_assign()))) {
         log_and_terminate("Unable to create shadow map DSV!");
      }
   }

   const D3D11_SHADER_RESOURCE_VIEW_DESC
      srv_desc{.Format = DXGI_FORMAT_R32_FLOAT,
               .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY,
               .Texture2DArray = {.MostDetailedMip = 0,
                                  .MipLevels = 1,
                                  .FirstArraySlice = 0,
                                  .ArraySize = cascade_count}};

   if (FAILED(_device->CreateShaderResourceView(_shadow_map_texture.get(), &srv_desc,
                                                _shadow_map_srv.clear_and_assign()))) {
      log_and_terminate("Unable to create shadow map SRV!");
   }

   _shadow_map_length_flt = static_cast<float>(length);
}

void Shadows_provider::draw_shadow_maps_instanced(ID3D11DeviceContext4& dc,
                                                  const Input_layout_descriptions& input_layout_descriptions,
                                                  effects::Profiler& profiler) noexcept
{
   Profile profile{profiler, dc, "Shadow Maps - Draw Instanced"sv};

   dc.ClearDepthStencilView(_shadow_map_dsv.get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

   auto* skins_srv = _skins_srv.get();
   auto* camera_cb = _camera_cb_buffer.get();
   auto* texture_sampler = _hardedged_texture_sampler.get();

   dc.VSSetShaderResources(0, 1, &skins_srv);
   dc.VSSetConstantBuffers(1, 1, &camera_cb);

   const D3D11_VIEWPORT viewport{.TopLeftX = 0.0f,
                                 .TopLeftY = 0.0f,
                                 .Width = _shadow_map_length_flt,
                                 .Height = _shadow_map_length_flt,
                                 .MinDepth = 0.0f,
                                 .MaxDepth = 1.0f};

   dc.RSSetViewports(1, &viewport);
   dc.RSSetState(_rasterizer_doublesided_state.get());
   dc.OMSetRenderTargets(0, nullptr, _shadow_map_dsv.get());

   dc.PSSetSamplers(0, 1, &texture_sampler);

   std::size_t transform_cb_offset = 0;
   D3D11_PRIMITIVE_TOPOLOGY current_primitive_topology =
      D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;

   const auto draw_meshes = [&](const std::vector<Meshes::Zprepass>& meshes,
                                ID3D11InputLayout* input_layout,
                                ID3D11VertexShader* vertex_shader) {
      if (meshes.empty()) return;

      dc.IASetInputLayout(input_layout);
      dc.VSSetShader(vertex_shader, nullptr, 0);

      for (std::size_t i = 0; i < meshes.size(); ++i) {
         auto& mesh = meshes[i];

         dc.IASetIndexBuffer(mesh.index_buffer.get(), DXGI_FORMAT_R16_UINT,
                             mesh.index_buffer_offset);

         auto* vertex_buffer = mesh.vertex_buffer.get();

         dc.IASetVertexBuffers(0, 1, &vertex_buffer, &mesh.vertex_buffer_stride,
                               &mesh.vertex_buffer_offset);

         auto* transform_cb = _transforms_cb_buffer.get();
         UINT first_constant = (sizeof(Transform_cb) / 16) * (i + transform_cb_offset);
         UINT num_constants = sizeof(Transform_cb) / 16;

         if (current_primitive_topology != mesh.primitive_topology) {
            dc.IASetPrimitiveTopology(mesh.primitive_topology);
            current_primitive_topology = mesh.primitive_topology;
         }

         dc.VSSetConstantBuffers1(0, 1, &transform_cb, &first_constant, &num_constants);

         dc.DrawIndexedInstanced(mesh.index_count, 4, mesh.start_index,
                                 mesh.base_vertex, 0);
      }

      transform_cb_offset += meshes.size();
   };

   draw_meshes(meshes.zprepass, _mesh_il.get(), _mesh_vs.get());
   draw_meshes(meshes.zprepass_compressed, _mesh_compressed_il.get(),
               _mesh_compressed_vs.get());
   draw_meshes(meshes.zprepass_skinned, _mesh_skinned_il.get(),
               _mesh_skinned_vs.get());
   draw_meshes(meshes.zprepass_compressed_skinned, _mesh_compressed_skinned_il.get(),
               _mesh_compressed_skinned_vs.get());

   const auto draw_hardedged_meshes = [&](const std::vector<Meshes::Zprepass_hardedged>& meshes,
                                          Hardedged_vertex_shader& vertex_shader) {
      if (meshes.empty()) return;

      dc.VSSetShader(vertex_shader.vs.get(), nullptr, 0);
      dc.PSSetShader(_mesh_hardedged_ps.get(), nullptr, 0);

      std::uint16_t current_input_layout = std::numeric_limits<std::uint16_t>::max();

      for (std::size_t i = 0; i < meshes.size(); ++i) {
         auto& mesh = meshes[i];

         if (current_input_layout != mesh.input_layout) {
            dc.IASetInputLayout(
               &vertex_shader.input_layouts.get(*_device, input_layout_descriptions,
                                                mesh.input_layout));

            current_input_layout = mesh.input_layout;
         }

         dc.IASetIndexBuffer(mesh.index_buffer.get(), DXGI_FORMAT_R16_UINT,
                             mesh.index_buffer_offset);

         auto* vertex_buffer = mesh.vertex_buffer.get();

         dc.IASetVertexBuffers(0, 1, &vertex_buffer, &mesh.vertex_buffer_stride,
                               &mesh.vertex_buffer_offset);

         auto* transform_cb = _transforms_cb_buffer.get();
         UINT first_constant = (sizeof(Transform_cb) / 16) * (i + transform_cb_offset);
         UINT num_constants = sizeof(Transform_cb) / 16;

         if (current_primitive_topology != mesh.primitive_topology) {
            dc.IASetPrimitiveTopology(mesh.primitive_topology);
            current_primitive_topology = mesh.primitive_topology;
         }

         dc.VSSetConstantBuffers1(0, 1, &transform_cb, &first_constant, &num_constants);

         auto* srv = mesh.texture.get();
         dc.PSSetShaderResources(0, 1, &srv);

         dc.DrawIndexedInstanced(mesh.index_count, 4, mesh.start_index,
                                 mesh.base_vertex, 0);
      }

      transform_cb_offset += meshes.size();
   };

   dc.RSSetState(_rasterizer_doublesided_state.get());

   draw_hardedged_meshes(meshes.zprepass_hardedged, _mesh_hardedged);
   draw_hardedged_meshes(meshes.zprepass_hardedged_compressed,
                         _mesh_hardedged_compressed);
   draw_hardedged_meshes(meshes.zprepass_hardedged_skinned, _mesh_hardedged_skinned);
   draw_hardedged_meshes(meshes.zprepass_hardedged_compressed_skinned,
                         _mesh_hardedged_compressed_skinned);
}

void Shadows_provider::draw_to_target_map(ID3D11DeviceContext4& dc,
                                          const Draw_args& args) noexcept
{
   Profile profile{args.profiler, dc, "Shadow Maps - Draw to Target"sv};

   std::array depth_srvs{&args.scene_depth, _shadow_map_srv.get()};
   auto* target_rtv = &args.target_map;
   auto* shadow_sampler_state = _shadow_map_sampler.get();
   auto* draw_to_target_cb = _draw_to_target_cb.get();

   const D3D11_VIEWPORT viewport{.TopLeftX = 0.0f,
                                 .TopLeftY = 0.0f,
                                 .Width = static_cast<float>(args.target_width),
                                 .Height = static_cast<float>(args.target_height),
                                 .MinDepth = 0.0f,
                                 .MaxDepth = 1.0f};

   dc.IASetInputLayout(nullptr);
   dc.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

   dc.VSSetShader(_draw_to_target_vs.get(), nullptr, 0);

   dc.RSSetViewports(1, &viewport);
   dc.RSSetState(nullptr);

   dc.OMSetDepthStencilState(_draw_to_target_depthstencil_state.get(), 0);
   dc.OMSetRenderTargets(1, &target_rtv, &args.scene_dsv);

   dc.PSSetSamplers(0, 1, &shadow_sampler_state);
   dc.PSSetShaderResources(0, depth_srvs.size(), depth_srvs.data());
   dc.PSSetConstantBuffers(0, 1, &draw_to_target_cb);
   dc.PSSetShader(_draw_to_target_ps.get(), nullptr, 0);

   dc.Draw(3, 0);
}

Shadows_provider::Hardedged_vertex_shader::Hardedged_vertex_shader(
   std::tuple<Com_ptr<ID3D11VertexShader>, shader::Bytecode_blob, shader::Vertex_input_layout> shader)
   : vs{std::get<Com_ptr<ID3D11VertexShader>>(shader)},
     input_layouts{std::get<shader::Vertex_input_layout>(shader),
                   std::get<shader::Bytecode_blob>(shader)}
{
}

}