
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

struct alignas(16) Camera_cb {
   glm::mat4 projection_matrix;
};

static_assert(sizeof(Camera_cb) == 64);

using Skin_buffer = std::array<std::array<glm::vec4, 3>, 15>;

static_assert(sizeof(Skin_buffer) == 720);

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

auto make_bounding_sphere(const auto& mesh) noexcept -> shadows::Bounding_sphere
{
   return {glm::vec3{mesh.world_matrix[0][3], mesh.world_matrix[1][3],
                     mesh.world_matrix[2][3]} +
              glm::vec3{mesh.position_decompress_add},
           glm::length(glm::vec3{mesh.position_decompress_mul} *
                       static_cast<float>(INT16_MAX))};
}

}

Shadows_provider::Shadows_provider(Com_ptr<ID3D11Device5> device,
                                   shader::Database& shaders,
                                   const Input_layout_descriptions& input_layout_descriptions) noexcept
   : _device{device},
     _input_layout_descriptions{input_layout_descriptions},
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

   _transforms_cb_buffer = [&] {
      Com_ptr<ID3D11Buffer> buffer;

      const D3D11_BUFFER_DESC desc{.ByteWidth =
                                      _transforms_cb_space * sizeof(Transform_cb),
                                   .Usage = D3D11_USAGE_DYNAMIC,
                                   .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
                                   .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE};

      if (FAILED(device->CreateBuffer(&desc, nullptr, buffer.clear_and_assign()))) {
         log_and_terminate("Unable to create shadow transforms buffer!");
      }

      return buffer;
   }();

   _skins_buffer = [&] {
      Com_ptr<ID3D11Buffer> buffer;

      const D3D11_BUFFER_DESC desc{.ByteWidth = _skins_space * sizeof(Skin_buffer),
                                   .Usage = D3D11_USAGE_DYNAMIC,
                                   .BindFlags = D3D11_BIND_SHADER_RESOURCE,
                                   .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
                                   .MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
                                   .StructureByteStride = sizeof(Skin_buffer)};

      if (FAILED(device->CreateBuffer(&desc, nullptr, buffer.clear_and_assign()))) {
         log_and_terminate("Unable to create shadow transforms buffer!");
      }

      return buffer;
   }();

   if (FAILED(_device->CreateShaderResourceView(_skins_buffer.get(), nullptr,
                                                _skins_srv.clear_and_assign()))) {
      log_and_terminate("Unable to create shadow skins SRV!");
   }

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

      const D3D11_RASTERIZER_DESC desc{
         .FillMode = D3D11_FILL_SOLID,
         .CullMode = D3D11_CULL_BACK,
         .FrontCounterClockwise = true,
         .DepthBiasClamp = _current_depth_bias_clamp,
         .SlopeScaledDepthBias = _current_slope_scaled_depth_bias,
      };

      if (FAILED(device->CreateRasterizerState(&desc,
                                               rasterizer_state.clear_and_assign()))) {
         log_and_terminate("Unable to create shadow rasterizer state!");
      }

      return rasterizer_state;
   }();

   _rasterizer_doublesided_state = [&] {
      Com_ptr<ID3D11RasterizerState> rasterizer_state;

      const D3D11_RASTERIZER_DESC desc{
         .FillMode = D3D11_FILL_SOLID,
         .CullMode = D3D11_CULL_NONE,
         .FrontCounterClockwise = true,
         .DepthBiasClamp = _current_depth_bias_clamp,
         .SlopeScaledDepthBias = _current_slope_scaled_depth_bias,
      };

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

   _meshes.uncompressed.reserve(128);
   _meshes.compressed.reserve(1024);
   _meshes.skinned.reserve(16);
   _meshes.compressed_skinned.reserve(128);
   _meshes.hardedged.reserve(128);
   _meshes.hardedged_compressed.reserve(128);
   _meshes.hardedged_skinned.reserve(4);
   _meshes.hardedged_compressed_skinned.reserve(4);

   _bounding_spheres.compressed.reserve(1024);
   _bounding_spheres.compressed_skinned.reserve(256);
   _bounding_spheres.hardedged_compressed.reserve(128);
   _bounding_spheres.hardedged_compressed_skinned.reserve(4);

   _visibility_lists.compressed.reserve(1024);
   _visibility_lists.compressed_skinned.reserve(256);
   _visibility_lists.hardedged_compressed.reserve(128);
   _visibility_lists.hardedged_compressed_skinned.reserve(4);
}

Shadows_provider::~Shadows_provider() = default;

void Shadows_provider::add_mesh(ID3D11DeviceContext4& dc, const Input_mesh& mesh)
{
   if (_drawn_shadow_maps) return;
   if (!_started_frame) begin_frame(dc);

   const UINT constants_index =
      add_transform_cb(dc, {
                              .world_matrix = mesh.world_matrix,
                           });

   _meshes.uncompressed.push_back({
      .primitive_topology = mesh.primitive_topology,

      .index_buffer = copy_raw_com_ptr(mesh.index_buffer),
      .index_buffer_offset = mesh.index_buffer_offset,

      .vertex_buffer = copy_raw_com_ptr(mesh.vertex_buffer),
      .vertex_buffer_offset = mesh.vertex_buffer_offset,
      .vertex_buffer_stride = mesh.vertex_buffer_stride,

      .index_count = mesh.index_count,
      .start_index = mesh.start_index,
      .base_vertex = mesh.base_vertex,

      .constants_index = constants_index,
   });
}

void Shadows_provider::add_mesh_compressed(ID3D11DeviceContext4& dc,
                                           const Input_mesh_compressed& mesh)
{
   if (_drawn_shadow_maps) return;
   if (!_started_frame) begin_frame(dc);

   const UINT constants_index =
      add_transform_cb(dc, {
                              .position_decompress_mul = mesh.position_decompress_mul,
                              .position_decompress_add = mesh.position_decompress_add,

                              .world_matrix = mesh.world_matrix,
                           });

   _meshes.compressed.push_back({
      .primitive_topology = mesh.primitive_topology,

      .index_buffer = copy_raw_com_ptr(mesh.index_buffer),
      .index_buffer_offset = mesh.index_buffer_offset,

      .vertex_buffer = copy_raw_com_ptr(mesh.vertex_buffer),
      .vertex_buffer_offset = mesh.vertex_buffer_offset,
      .vertex_buffer_stride = mesh.vertex_buffer_stride,

      .index_count = mesh.index_count,
      .start_index = mesh.start_index,
      .base_vertex = mesh.base_vertex,

      .constants_index = constants_index,
   });

   _bounding_spheres.compressed.emplace_back(make_bounding_sphere(mesh));
}

void Shadows_provider::add_mesh_skinned(
   ID3D11DeviceContext4& dc, const Input_mesh& mesh,
   const std::array<std::array<glm::vec4, 3>, 15>& bone_matrices)
{
   if (_drawn_shadow_maps) return;
   if (!_started_frame) begin_frame(dc);

   const UINT skin_index = add_skin(dc, bone_matrices);
   const UINT constants_index =
      add_transform_cb(dc, {
                              .skin_index = skin_index,

                              .world_matrix = mesh.world_matrix,
                           });

   _meshes.skinned.push_back({
      .primitive_topology = mesh.primitive_topology,

      .index_buffer = copy_raw_com_ptr(mesh.index_buffer),
      .index_buffer_offset = mesh.index_buffer_offset,

      .vertex_buffer = copy_raw_com_ptr(mesh.vertex_buffer),
      .vertex_buffer_offset = mesh.vertex_buffer_offset,
      .vertex_buffer_stride = mesh.vertex_buffer_stride,

      .index_count = mesh.index_count,
      .start_index = mesh.start_index,
      .base_vertex = mesh.base_vertex,

      .constants_index = constants_index,
   });
}

void Shadows_provider::add_mesh_compressed_skinned(
   ID3D11DeviceContext4& dc, const Input_mesh_compressed& mesh,
   const std::array<std::array<glm::vec4, 3>, 15>& bone_matrices)
{
   if (_drawn_shadow_maps) return;
   if (!_started_frame) begin_frame(dc);

   const UINT skin_index = add_skin(dc, bone_matrices);
   const UINT constants_index =
      add_transform_cb(dc, {
                              .position_decompress_mul = mesh.position_decompress_mul,
                              .skin_index = skin_index,
                              .position_decompress_add = mesh.position_decompress_add,

                              .world_matrix = mesh.world_matrix,
                           });

   _meshes.compressed_skinned.push_back({
      .primitive_topology = mesh.primitive_topology,

      .index_buffer = copy_raw_com_ptr(mesh.index_buffer),
      .index_buffer_offset = mesh.index_buffer_offset,

      .vertex_buffer = copy_raw_com_ptr(mesh.vertex_buffer),
      .vertex_buffer_offset = mesh.vertex_buffer_offset,
      .vertex_buffer_stride = mesh.vertex_buffer_stride,

      .index_count = mesh.index_count,
      .start_index = mesh.start_index,
      .base_vertex = mesh.base_vertex,

      .constants_index = constants_index,
   });

   _bounding_spheres.compressed_skinned.emplace_back(make_bounding_sphere(mesh));
}

void Shadows_provider::add_mesh_hardedged(ID3D11DeviceContext4& dc,
                                          const Input_hardedged_mesh& mesh)
{
   if (_drawn_shadow_maps) return;
   if (!_started_frame) begin_frame(dc);

   if (config.disable_dynamic_hardedged_meshes) {
      D3D11_BUFFER_DESC desc{};

      mesh.vertex_buffer.GetDesc(&desc);

      if (desc.Usage == D3D11_USAGE_DYNAMIC) return;
   }

   const UINT constants_index =
      add_transform_cb(dc, {
                              .world_matrix = mesh.world_matrix,

                              .x_texcoord_transform = mesh.x_texcoord_transform,
                              .y_texcoord_transform = mesh.y_texcoord_transform,
                           });

   _meshes.hardedged.push_back({
      .input_layout = &_mesh_hardedged.input_layouts.get(*_device, _input_layout_descriptions,
                                                         mesh.input_layout),

      .primitive_topology = mesh.primitive_topology,

      .index_buffer = copy_raw_com_ptr(mesh.index_buffer),
      .index_buffer_offset = mesh.index_buffer_offset,

      .vertex_buffer = copy_raw_com_ptr(mesh.vertex_buffer),
      .vertex_buffer_offset = mesh.vertex_buffer_offset,
      .vertex_buffer_stride = mesh.vertex_buffer_stride,

      .index_count = mesh.index_count,
      .start_index = mesh.start_index,
      .base_vertex = mesh.base_vertex,

      .constants_index = constants_index,
      .texture = copy_raw_com_ptr(mesh.texture),
   });
}

void Shadows_provider::add_mesh_hardedged_compressed(
   ID3D11DeviceContext4& dc, const Input_mesh_hardedged_compressed& mesh)
{
   if (_drawn_shadow_maps) return;
   if (!_started_frame) begin_frame(dc);

   if (config.disable_dynamic_hardedged_meshes) {
      D3D11_BUFFER_DESC desc{};

      mesh.vertex_buffer.GetDesc(&desc);

      if (desc.Usage == D3D11_USAGE_DYNAMIC) return;
   }

   const UINT constants_index =
      add_transform_cb(dc, {
                              .position_decompress_mul = mesh.position_decompress_mul,
                              .position_decompress_add = mesh.position_decompress_add,

                              .world_matrix = mesh.world_matrix,

                              .x_texcoord_transform = mesh.x_texcoord_transform,
                              .y_texcoord_transform = mesh.y_texcoord_transform,
                           });

   _meshes.hardedged_compressed.push_back({
      .input_layout =
         &_mesh_hardedged_compressed.input_layouts.get(*_device, _input_layout_descriptions,
                                                       mesh.input_layout),

      .primitive_topology = mesh.primitive_topology,

      .index_buffer = copy_raw_com_ptr(mesh.index_buffer),
      .index_buffer_offset = mesh.index_buffer_offset,

      .vertex_buffer = copy_raw_com_ptr(mesh.vertex_buffer),
      .vertex_buffer_offset = mesh.vertex_buffer_offset,
      .vertex_buffer_stride = mesh.vertex_buffer_stride,

      .index_count = mesh.index_count,
      .start_index = mesh.start_index,
      .base_vertex = mesh.base_vertex,

      .constants_index = constants_index,
      .texture = copy_raw_com_ptr(mesh.texture),
   });

   _bounding_spheres.hardedged_compressed.emplace_back(make_bounding_sphere(mesh));
}

void Shadows_provider::add_mesh_hardedged_skinned(
   ID3D11DeviceContext4& dc, const Input_hardedged_mesh& mesh,
   const std::array<std::array<glm::vec4, 3>, 15>& bone_matrices)
{
   if (_drawn_shadow_maps) return;
   if (!_started_frame) begin_frame(dc);

   if (config.disable_dynamic_hardedged_meshes) {
      D3D11_BUFFER_DESC desc{};

      mesh.vertex_buffer.GetDesc(&desc);

      if (desc.Usage == D3D11_USAGE_DYNAMIC) return;
   }

   const UINT skin_index = add_skin(dc, bone_matrices);
   const UINT constants_index =
      add_transform_cb(dc, {
                              .skin_index = skin_index,

                              .world_matrix = mesh.world_matrix,

                              .x_texcoord_transform = mesh.x_texcoord_transform,
                              .y_texcoord_transform = mesh.y_texcoord_transform,
                           });

   _meshes.hardedged_skinned.push_back({
      .input_layout = &_mesh_hardedged_skinned.input_layouts.get(*_device, _input_layout_descriptions,
                                                                 mesh.input_layout),
      .primitive_topology = mesh.primitive_topology,

      .index_buffer = copy_raw_com_ptr(mesh.index_buffer),
      .index_buffer_offset = mesh.index_buffer_offset,

      .vertex_buffer = copy_raw_com_ptr(mesh.vertex_buffer),
      .vertex_buffer_offset = mesh.vertex_buffer_offset,
      .vertex_buffer_stride = mesh.vertex_buffer_stride,

      .index_count = mesh.index_count,
      .start_index = mesh.start_index,
      .base_vertex = mesh.base_vertex,

      .constants_index = constants_index,
      .texture = copy_raw_com_ptr(mesh.texture),
   });
}

void Shadows_provider::add_mesh_hardedged_compressed_skinned(
   ID3D11DeviceContext4& dc, const Input_mesh_hardedged_compressed& mesh,
   const std::array<std::array<glm::vec4, 3>, 15>& bone_matrices)
{
   if (_drawn_shadow_maps) return;
   if (!_started_frame) begin_frame(dc);

   if (config.disable_dynamic_hardedged_meshes) {
      D3D11_BUFFER_DESC desc{};

      mesh.vertex_buffer.GetDesc(&desc);

      if (desc.Usage == D3D11_USAGE_DYNAMIC) return;
   }

   const UINT skin_index = add_skin(dc, bone_matrices);
   const UINT constants_index =
      add_transform_cb(dc, {
                              .position_decompress_mul = mesh.position_decompress_mul,
                              .skin_index = skin_index,
                              .position_decompress_add = mesh.position_decompress_add,

                              .world_matrix = mesh.world_matrix,

                              .x_texcoord_transform = mesh.x_texcoord_transform,
                              .y_texcoord_transform = mesh.y_texcoord_transform,
                           });

   _meshes.hardedged_compressed_skinned.push_back({
      .input_layout =
         &_mesh_hardedged_compressed_skinned.input_layouts.get(*_device, _input_layout_descriptions,
                                                               mesh.input_layout),

      .primitive_topology = mesh.primitive_topology,

      .index_buffer = copy_raw_com_ptr(mesh.index_buffer),
      .index_buffer_offset = mesh.index_buffer_offset,

      .vertex_buffer = copy_raw_com_ptr(mesh.vertex_buffer),
      .vertex_buffer_offset = mesh.vertex_buffer_offset,
      .vertex_buffer_stride = mesh.vertex_buffer_stride,

      .index_count = mesh.index_count,
      .start_index = mesh.start_index,
      .base_vertex = mesh.base_vertex,

      .constants_index = constants_index,
      .texture = copy_raw_com_ptr(mesh.texture),
   });

   _bounding_spheres.hardedged_compressed_skinned.emplace_back(
      make_bounding_sphere(mesh));
}

void Shadows_provider::draw_shadow_maps(ID3D11DeviceContext4& dc,
                                        const Draw_args& args) noexcept
{
   Profile profile{args.profiler, dc, "Shadow Maps - Draw Cascades"sv};

   prepare_draw_shadow_maps(dc, args);

   dc.ClearState();

   dc.ClearDepthStencilView(_shadow_map_dsv.get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

   draw_shadow_maps_cascades(dc);

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

   shadows::shadow_world.draw_shadow_views(dc,
                                           {
                                              .depth_bias_clamp = _current_depth_bias_clamp,
                                              .slope_scaled_depth_bias =
                                                 _current_slope_scaled_depth_bias,
                                           },
                                           shadow_views);

   draw_to_target_map(dc, args);

   dc.DiscardResource(_shadow_map_texture.get());

   _drawn_shadow_maps = true;
}

void Shadows_provider::end_frame(ID3D11DeviceContext4& dc) noexcept
{
   if (_started_frame && !_drawn_shadow_maps) {
      dc.Unmap(_transforms_cb_buffer.get(), 0);
      dc.Unmap(_skins_buffer.get(), 0);
   }

   _drawn_shadow_maps = false;
   _started_frame = false;
}

void Shadows_provider::begin_frame(ID3D11DeviceContext4& dc) noexcept
{
   _meshes.clear();
   _bounding_spheres.clear();

   _used_transforms = 0;
   _used_skins = 0;

   D3D11_MAPPED_SUBRESOURCE mapped{};

   if (FAILED(dc.Map(_transforms_cb_buffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0,
                     &mapped))) {
      log_and_terminate("Failed to map shadows transforms buffer!");
   }

   _transforms_cb_upload = static_cast<std::byte*>(mapped.pData);

   if (FAILED(dc.Map(_skins_buffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
      log_and_terminate("Failed to map shadows skins buffer!");
   }

   _skins_buffer_upload = static_cast<std::byte*>(mapped.pData);

   if (config.hw_depth_bias != _current_depth_bias ||
       config.hw_depth_bias_clamp != _current_depth_bias_clamp ||
       config.hw_slope_scaled_depth_bias != _current_slope_scaled_depth_bias) {
      _current_depth_bias = config.hw_depth_bias;
      _current_depth_bias_clamp = config.hw_depth_bias_clamp;
      _current_slope_scaled_depth_bias = config.hw_slope_scaled_depth_bias;

      D3D11_RASTERIZER_DESC desc{
         .FillMode = D3D11_FILL_SOLID,
         .CullMode = D3D11_CULL_BACK,
         .FrontCounterClockwise = true,
         .DepthBias = _current_depth_bias,
         .DepthBiasClamp = _current_depth_bias_clamp,
         .SlopeScaledDepthBias = _current_slope_scaled_depth_bias,
      };

      if (FAILED(_device->CreateRasterizerState(&desc,
                                                _rasterizer_state.clear_and_assign()))) {
         log_and_terminate("Unable to create shadow rasterizer state!");
      }

      desc.CullMode = D3D11_CULL_NONE;

      if (FAILED(_device->CreateRasterizerState(&desc, _rasterizer_doublesided_state
                                                          .clear_and_assign()))) {
         log_and_terminate("Unable to create shadow rasterizer state!");
      }
   }

   _started_frame = true;
}

void Shadows_provider::prepare_draw_shadow_maps(ID3D11DeviceContext4& dc,
                                                const Draw_args& args) noexcept
{
   build_cascade_info();

   upload_buffer_data(dc, args);

   _transforms_cb_upload = nullptr;
   dc.Unmap(_transforms_cb_buffer.get(), 0);

   _skins_buffer_upload = nullptr;
   dc.Unmap(_skins_buffer.get(), 0);
}

void Shadows_provider::build_cascade_info() noexcept
{
   auto cascades = make_shadow_cascades(glm::normalize(-light_direction),
                                        view_proj_matrix, config.start_depth,
                                        config.end_depth, _shadow_map_length_flt);

   _cascade_view_proj_matrices = {cascades[0].view_projection_matrix(),
                                  cascades[1].view_projection_matrix(),
                                  cascades[2].view_projection_matrix(),
                                  cascades[3].view_projection_matrix()};

   _cascade_texture_matrices = {cascades[0].texture_matrix(),
                                cascades[1].texture_matrix(),
                                cascades[2].texture_matrix(),
                                cascades[3].texture_matrix()};
}

void Shadows_provider::upload_buffer_data(ID3D11DeviceContext4& dc,
                                          const Draw_args& args) noexcept
{
   upload_draw_to_target_buffer(dc, args);
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

auto Shadows_provider::count_active_meshes() const noexcept -> std::size_t
{
   return _meshes.uncompressed.size() + _meshes.compressed.size() +
          _meshes.skinned.size() + _meshes.compressed_skinned.size() +
          _meshes.hardedged.size() + _meshes.hardedged_compressed.size() +
          _meshes.hardedged_skinned.size() +
          _meshes.hardedged_compressed_skinned.size();
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

void Shadows_provider::draw_shadow_maps_cascades(ID3D11DeviceContext4& dc) noexcept
{
   const D3D11_VIEWPORT viewport{.Width = _shadow_map_length_flt,
                                 .Height = _shadow_map_length_flt,
                                 .MinDepth = 0.0f,
                                 .MaxDepth = 1.0f};

   dc.VSSetShaderResources(0, 1, _skins_srv.get_ptr());

   dc.RSSetViewports(1, &viewport);

   dc.PSSetSamplers(0, 1, _hardedged_texture_sampler.get_ptr());

   for (std::uint32_t view_index = 0; view_index < _shadow_map_dsvs.size();
        ++view_index) {
      fill_visibility_lists(_cascade_view_proj_matrices[view_index]);

      update_dynamic_buffer(dc, *_camera_cb_buffer,
                            Camera_cb{.projection_matrix =
                                         _cascade_view_proj_matrices[view_index]});

      dc.VSSetConstantBuffers(1, 1, _camera_cb_buffer.get_ptr());

      dc.RSSetState(_rasterizer_state.get());

      dc.OMSetRenderTargets(0, nullptr, _shadow_map_dsvs[view_index].get());

      D3D11_PRIMITIVE_TOPOLOGY current_primitive_topology =
         D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;

      const auto draw_meshes = [&](const std::vector<Mesh>& meshes,
                                   ID3D11InputLayout* input_layout,
                                   ID3D11VertexShader* vertex_shader) {
         if (meshes.empty()) return;

         dc.IASetInputLayout(input_layout);
         dc.VSSetShader(vertex_shader, nullptr, 0);
         dc.PSSetShader(nullptr, nullptr, 0);

         for (const Mesh& mesh : meshes) {
            dc.IASetIndexBuffer(mesh.index_buffer.get(), DXGI_FORMAT_R16_UINT,
                                mesh.index_buffer_offset);

            auto* vertex_buffer = mesh.vertex_buffer.get();

            dc.IASetVertexBuffers(0, 1, &vertex_buffer, &mesh.vertex_buffer_stride,
                                  &mesh.vertex_buffer_offset);

            auto* transform_cb = _transforms_cb_buffer.get();
            UINT first_constant = (sizeof(Transform_cb) / 16) * mesh.constants_index;
            UINT num_constants = sizeof(Transform_cb) / 16;

            if (current_primitive_topology != mesh.primitive_topology) {
               dc.IASetPrimitiveTopology(mesh.primitive_topology);
               current_primitive_topology = mesh.primitive_topology;
            }

            dc.VSSetConstantBuffers1(0, 1, &transform_cb, &first_constant,
                                     &num_constants);

            dc.DrawIndexed(mesh.index_count, mesh.start_index, mesh.base_vertex);
         }
      };

      draw_meshes(_meshes.uncompressed, _mesh_il.get(), _mesh_vs.get());
      draw_meshes(_meshes.skinned, _mesh_skinned_il.get(), _mesh_skinned_vs.get());

      const auto draw_culled_meshes = [&](const std::vector<std::uint32_t>& visible,
                                          const std::vector<Mesh>& meshes,
                                          ID3D11InputLayout* input_layout,
                                          ID3D11VertexShader* vertex_shader) {
         if (visible.empty()) return;

         dc.IASetInputLayout(input_layout);
         dc.VSSetShader(vertex_shader, nullptr, 0);
         dc.PSSetShader(nullptr, nullptr, 0);

         for (const std::uint32_t i : visible) {
            const Mesh& mesh = meshes[i];

            dc.IASetIndexBuffer(mesh.index_buffer.get(), DXGI_FORMAT_R16_UINT,
                                mesh.index_buffer_offset);

            auto* vertex_buffer = mesh.vertex_buffer.get();

            dc.IASetVertexBuffers(0, 1, &vertex_buffer, &mesh.vertex_buffer_stride,
                                  &mesh.vertex_buffer_offset);

            auto* transform_cb = _transforms_cb_buffer.get();
            UINT first_constant = (sizeof(Transform_cb) / 16) * mesh.constants_index;
            UINT num_constants = sizeof(Transform_cb) / 16;

            if (current_primitive_topology != mesh.primitive_topology) {
               dc.IASetPrimitiveTopology(mesh.primitive_topology);
               current_primitive_topology = mesh.primitive_topology;
            }

            dc.VSSetConstantBuffers1(0, 1, &transform_cb, &first_constant,
                                     &num_constants);

            dc.DrawIndexed(mesh.index_count, mesh.start_index, mesh.base_vertex);
         }
      };

      draw_culled_meshes(_visibility_lists.compressed, _meshes.compressed,
                         _mesh_compressed_il.get(), _mesh_compressed_vs.get());
      draw_culled_meshes(_visibility_lists.compressed_skinned, _meshes.compressed_skinned,
                         _mesh_compressed_skinned_il.get(),
                         _mesh_compressed_skinned_vs.get());

      const auto draw_hardedged_meshes = [&](const std::vector<Mesh_hardedged>& meshes,
                                             Hardedged_vertex_shader& vertex_shader) {
         if (meshes.empty()) return;

         dc.VSSetShader(vertex_shader.vs.get(), nullptr, 0);
         dc.PSSetShader(_mesh_hardedged_ps.get(), nullptr, 0);

         ID3D11InputLayout* current_input_layout = nullptr;

         for (const Mesh_hardedged& mesh : meshes) {
            if (current_input_layout != mesh.input_layout) {
               dc.IASetInputLayout(mesh.input_layout);

               current_input_layout = mesh.input_layout;
            }

            dc.IASetIndexBuffer(mesh.index_buffer.get(), DXGI_FORMAT_R16_UINT,
                                mesh.index_buffer_offset);

            auto* vertex_buffer = mesh.vertex_buffer.get();

            dc.IASetVertexBuffers(0, 1, &vertex_buffer, &mesh.vertex_buffer_stride,
                                  &mesh.vertex_buffer_offset);

            auto* transform_cb = _transforms_cb_buffer.get();
            UINT first_constant = (sizeof(Transform_cb) / 16) * mesh.constants_index;
            UINT num_constants = sizeof(Transform_cb) / 16;

            if (current_primitive_topology != mesh.primitive_topology) {
               dc.IASetPrimitiveTopology(mesh.primitive_topology);
               current_primitive_topology = mesh.primitive_topology;
            }

            dc.VSSetConstantBuffers1(0, 1, &transform_cb, &first_constant,
                                     &num_constants);

            auto* srv = mesh.texture.get();
            dc.PSSetShaderResources(0, 1, &srv);

            dc.DrawIndexed(mesh.index_count, mesh.start_index, mesh.base_vertex);
         }
      };

      dc.RSSetState(_rasterizer_doublesided_state.get());

      draw_hardedged_meshes(_meshes.hardedged, _mesh_hardedged);
      draw_hardedged_meshes(_meshes.hardedged_skinned, _mesh_hardedged_skinned);

      const auto draw_culled_hardedged_meshes =
         [&](const std::vector<std::uint32_t>& visible,
             const std::vector<Mesh_hardedged>& meshes,
             Hardedged_vertex_shader& vertex_shader) {
            if (visible.empty()) return;

            dc.VSSetShader(vertex_shader.vs.get(), nullptr, 0);
            dc.PSSetShader(_mesh_hardedged_ps.get(), nullptr, 0);

            ID3D11InputLayout* current_input_layout = nullptr;

            for (const std::size_t i : visible) {
               const Mesh_hardedged& mesh = meshes[i];

               if (current_input_layout != mesh.input_layout) {
                  dc.IASetInputLayout(mesh.input_layout);

                  current_input_layout = mesh.input_layout;
               }

               dc.IASetIndexBuffer(mesh.index_buffer.get(), DXGI_FORMAT_R16_UINT,
                                   mesh.index_buffer_offset);

               auto* vertex_buffer = mesh.vertex_buffer.get();

               dc.IASetVertexBuffers(0, 1, &vertex_buffer, &mesh.vertex_buffer_stride,
                                     &mesh.vertex_buffer_offset);

               auto* transform_cb = _transforms_cb_buffer.get();
               UINT first_constant = (sizeof(Transform_cb) / 16) * mesh.constants_index;
               UINT num_constants = sizeof(Transform_cb) / 16;

               if (current_primitive_topology != mesh.primitive_topology) {
                  dc.IASetPrimitiveTopology(mesh.primitive_topology);
                  current_primitive_topology = mesh.primitive_topology;
               }

               dc.VSSetConstantBuffers1(0, 1, &transform_cb, &first_constant,
                                        &num_constants);

               auto* srv = mesh.texture.get();
               dc.PSSetShaderResources(0, 1, &srv);

               dc.DrawIndexed(mesh.index_count, mesh.start_index, mesh.base_vertex);
            }
         };

      draw_culled_hardedged_meshes(_visibility_lists.hardedged_compressed,
                                   _meshes.hardedged_compressed,
                                   _mesh_hardedged_compressed);
      draw_culled_hardedged_meshes(_visibility_lists.hardedged_compressed_skinned,
                                   _meshes.hardedged_compressed_skinned,
                                   _mesh_hardedged_compressed_skinned);

      ImGui::Text("z: %u, zs: %u, zh: %u, zhs: %u",
                  _visibility_lists.compressed.size(),
                  _visibility_lists.compressed_skinned.size(),
                  _visibility_lists.hardedged_compressed.size(),
                  _visibility_lists.hardedged_compressed_skinned.size());
   }
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

void Shadows_provider::fill_visibility_lists(const glm::mat4& projection_matrix) noexcept
{
   _visibility_lists.clear();

   shadows::Frustum frustum{glm::inverse(projection_matrix)};

   _visibility_lists.compressed.reserve(_meshes.compressed.size());
   _visibility_lists.compressed_skinned.reserve(_meshes.compressed_skinned.size());
   _visibility_lists.hardedged_compressed.reserve(_meshes.hardedged_compressed.size());
   _visibility_lists.hardedged_compressed_skinned.reserve(
      _meshes.hardedged_compressed_skinned.size());

   for (std::uint32_t i = 0; i < _meshes.compressed.size(); ++i) {
      if (!shadows::intersects(frustum, _bounding_spheres.compressed[i]))
         continue;

      _visibility_lists.compressed.push_back(i);
   }

   for (std::uint32_t i = 0; i < _meshes.compressed_skinned.size(); ++i) {
      if (!shadows::intersects(frustum, _bounding_spheres.compressed_skinned[i]))
         continue;

      _visibility_lists.compressed_skinned.push_back(i);
   }

   for (std::uint32_t i = 0; i < _meshes.hardedged_compressed.size(); ++i) {
      if (!shadows::intersects(frustum, _bounding_spheres.hardedged_compressed[i]))
         continue;

      _visibility_lists.hardedged_compressed.push_back(i);
   }

   for (std::uint32_t i = 0; i < _meshes.hardedged_compressed_skinned.size(); ++i) {
      if (!shadows::intersects(frustum,
                               _bounding_spheres.hardedged_compressed_skinned[i]))
         continue;

      _visibility_lists.hardedged_compressed_skinned.push_back(i);
   }
}

auto Shadows_provider::add_transform_cb(ID3D11DeviceContext4& dc,
                                        const Transform_cb& cb) noexcept -> UINT
{
   if (_used_transforms == _transforms_cb_space) {
      _transforms_cb_upload = nullptr;

      dc.Unmap(_transforms_cb_buffer.get(), 0);

      resize_transform_cb_buffer(dc, _transforms_cb_space + 1);

      D3D11_MAPPED_SUBRESOURCE mapped{};

      if (FAILED(dc.Map(_transforms_cb_buffer.get(), 0,
                        D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mapped))) {
         log_and_terminate("Failed to map shadows transforms buffer!");
      }

      _transforms_cb_upload = static_cast<std::byte*>(mapped.pData);
   }

   const UINT index = _used_transforms;

   std::memcpy(_transforms_cb_upload + index * sizeof(Transform_cb), &cb,
               sizeof(Transform_cb));

   _used_transforms += 1;

   return index;
}

void Shadows_provider::resize_transform_cb_buffer(ID3D11DeviceContext4& dc,
                                                  std::size_t needed_space) noexcept
{
   const UINT old_transforms_cb_space = _transforms_cb_space;
   const Com_ptr<ID3D11Buffer> transforms_cb_buffer_old =
      std::move(_transforms_cb_buffer);

   _transforms_cb_space = needed_space + (needed_space / 2);

   const D3D11_BUFFER_DESC desc{.ByteWidth = _transforms_cb_space * sizeof(Transform_cb),
                                .Usage = D3D11_USAGE_DYNAMIC,
                                .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
                                .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE};

   if (FAILED(_device->CreateBuffer(&desc, nullptr,
                                    _transforms_cb_buffer.clear_and_assign()))) {
      log_and_terminate("Unable to create shadow transforms buffer!");
   }

   const D3D11_BOX copy_box{
      .right = old_transforms_cb_space * sizeof(Transform_cb),
      .bottom = 1,
      .back = 1,
   };

   dc.CopySubresourceRegion1(_transforms_cb_buffer.get(), 0, 0, 0, 0,
                             transforms_cb_buffer_old.get(), 0, &copy_box,
                             D3D11_COPY_NO_OVERWRITE);
}

auto Shadows_provider::add_skin(ID3D11DeviceContext4& dc,
                                const std::array<std::array<glm::vec4, 3>, 15>& skin) noexcept
   -> UINT
{
   if (_used_skins == _skins_space) {
      _skins_buffer_upload = nullptr;

      dc.Unmap(_skins_buffer.get(), 0);

      resize_skins_buffer(dc, _skins_space + 1);

      D3D11_MAPPED_SUBRESOURCE mapped{};

      if (FAILED(dc.Map(_skins_buffer.get(), 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0,
                        &mapped))) {
         log_and_terminate("Failed to map shadows transforms buffer!");
      }

      _skins_buffer_upload = static_cast<std::byte*>(mapped.pData);
   }

   const UINT index = _used_skins;

   std::memcpy(_skins_buffer_upload + index * sizeof(skin), &skin, sizeof(skin));

   _used_skins += 1;

   return index;
}

void Shadows_provider::resize_skins_buffer(ID3D11DeviceContext4& dc,
                                           std::size_t needed_space) noexcept
{
   const UINT old_skins_space = _skins_space;
   const Com_ptr<ID3D11Buffer> skins_buffer_old = std::move(_skins_buffer);

   _skins_space = needed_space + (needed_space / 2);

   const D3D11_BUFFER_DESC desc{.ByteWidth = _skins_space * sizeof(Skin_buffer),
                                .Usage = D3D11_USAGE_DYNAMIC,
                                .BindFlags = D3D11_BIND_SHADER_RESOURCE,
                                .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
                                .MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
                                .StructureByteStride = sizeof(Skin_buffer)};

   if (FAILED(_device->CreateBuffer(&desc, nullptr, _skins_buffer.clear_and_assign()))) {
      log_and_terminate("Unable to create shadow skins buffer!");
   }

   if (FAILED(_device->CreateShaderResourceView(_skins_buffer.get(), nullptr,
                                                _skins_srv.clear_and_assign()))) {
      log_and_terminate("Unable to create shadow skins SRV!");
   }

   const D3D11_BOX copy_box{
      .right = old_skins_space * sizeof(Skin_buffer),
      .bottom = 1,
      .back = 1,
   };

   dc.CopySubresourceRegion1(_skins_buffer.get(), 0, 0, 0, 0, skins_buffer_old.get(),
                             0, &copy_box, D3D11_COPY_NO_OVERWRITE);
}

Shadows_provider::Hardedged_vertex_shader::Hardedged_vertex_shader(
   std::tuple<Com_ptr<ID3D11VertexShader>, shader::Bytecode_blob, shader::Vertex_input_layout> shader)
   : vs{std::get<Com_ptr<ID3D11VertexShader>>(shader)},
     input_layouts{std::get<shader::Vertex_input_layout>(shader),
                   std::get<shader::Bytecode_blob>(shader)}
{
}

Shadows_provider::Bounding_spheres::Bounding_spheres() = default;

Shadows_provider::Bounding_spheres::~Bounding_spheres() = default;

void Shadows_provider::Meshes::clear() noexcept
{
   uncompressed.clear();
   compressed.clear();
   skinned.clear();
   compressed_skinned.clear();
   hardedged.clear();
   hardedged_compressed.clear();
   hardedged_skinned.clear();
   hardedged_compressed_skinned.clear();
}

void Shadows_provider::Bounding_spheres::clear() noexcept
{
   compressed.clear();
   compressed_skinned.clear();
   hardedged_compressed.clear();
   hardedged_compressed_skinned.clear();
}

void Shadows_provider::Visibility_lists::clear() noexcept
{
   compressed.clear();
   compressed_skinned.clear();
   hardedged_compressed.clear();
   hardedged_compressed_skinned.clear();
}

}