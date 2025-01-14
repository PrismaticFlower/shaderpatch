#include "debug_model_draw.hpp"

#include "../../logger.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace sp::shadows {

namespace {

struct Constants {
   glm::vec3 position_decompress_mul;
   std::uint32_t pad0;

   glm::vec3 position_decompress_add;
   std::uint32_t pad1;

   glm::mat4 projection_matrix;
};

// Shader for below bytecode
// Compiled with:
// VS: fxc /T vs_4_0 /E main_vs /Fh shader.h <shader>.hlsl
// PS: fxc /T ps_4_0 /E main_ps /Fh shader.h <shader>.hlsl
#if 0

cbuffer Constants {
   float3 position_decompress_mul;
   float3 position_decompress_add;

   float4x4 projection_matrix;
};

struct Vertex_output {
   float3 positionOS : POSITIONOS;
   float4 positionPS : SV_Position;
};

Vertex_output main_vs(int position_XCS : POSITION_X, int position_YCS : POSITION_Y, int position_ZCS : POSITION_Z)
{
   int3 positionCS = {position_XCS, position_YCS, position_ZCS};
   float3 positionOS = positionCS.xyz * position_decompress_mul + position_decompress_add;

   Vertex_output output;

   output.positionOS = positionOS;
   output.positionPS = mul(float4(positionOS, 1.0), projection_matrix);

   return output;
}

float4 main_ps(Vertex_output input) : SV_Target0
{   
   const float3 light_normalOS = normalize(float3(159.264923, -300.331013, 66.727310));
   float3 normalOS = normalize(cross(ddx(input.positionOS), ddy(input.positionOS)));

   float3 light = sqrt(saturate(dot(normalize(normalOS), light_normalOS)));

   return float4(light * 0.6 + (1.0 - light) * 0.25.xxx, 1.0f);
}

#endif

const BYTE g_main_vs[] =
   {68,  88,  66,  67,  193, 212, 8,   39,  50,  100, 63,  189, 150, 195, 185,
    218, 97,  199, 117, 210, 1,   0,   0,   0,   60,  4,   0,   0,   5,   0,
    0,   0,   52,  0,   0,   0,   112, 1,   0,   0,   236, 1,   0,   0,   68,
    2,   0,   0,   192, 3,   0,   0,   82,  68,  69,  70,  52,  1,   0,   0,
    1,   0,   0,   0,   72,  0,   0,   0,   1,   0,   0,   0,   28,  0,   0,
    0,   0,   4,   254, 255, 0,   1,   0,   0,   12,  1,   0,   0,   60,  0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,
    67,  111, 110, 115, 116, 97,  110, 116, 115, 0,   171, 171, 60,  0,   0,
    0,   3,   0,   0,   0,   96,  0,   0,   0,   96,  0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   168, 0,   0,   0,   0,   0,   0,   0,   12,
    0,   0,   0,   2,   0,   0,   0,   192, 0,   0,   0,   0,   0,   0,   0,
    208, 0,   0,   0,   16,  0,   0,   0,   12,  0,   0,   0,   2,   0,   0,
    0,   192, 0,   0,   0,   0,   0,   0,   0,   232, 0,   0,   0,   32,  0,
    0,   0,   64,  0,   0,   0,   2,   0,   0,   0,   252, 0,   0,   0,   0,
    0,   0,   0,   112, 111, 115, 105, 116, 105, 111, 110, 95,  100, 101, 99,
    111, 109, 112, 114, 101, 115, 115, 95,  109, 117, 108, 0,   1,   0,   3,
    0,   1,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   112, 111,
    115, 105, 116, 105, 111, 110, 95,  100, 101, 99,  111, 109, 112, 114, 101,
    115, 115, 95,  97,  100, 100, 0,   112, 114, 111, 106, 101, 99,  116, 105,
    111, 110, 95,  109, 97,  116, 114, 105, 120, 0,   171, 171, 3,   0,   3,
    0,   4,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   77,  105,
    99,  114, 111, 115, 111, 102, 116, 32,  40,  82,  41,  32,  72,  76,  83,
    76,  32,  83,  104, 97,  100, 101, 114, 32,  67,  111, 109, 112, 105, 108,
    101, 114, 32,  49,  48,  46,  49,  0,   73,  83,  71,  78,  116, 0,   0,
    0,   3,   0,   0,   0,   8,   0,   0,   0,   80,  0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   2,   0,   0,   0,   0,   0,   0,   0,   1,
    1,   0,   0,   91,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    2,   0,   0,   0,   1,   0,   0,   0,   1,   1,   0,   0,   102, 0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,   2,   0,
    0,   0,   1,   1,   0,   0,   80,  79,  83,  73,  84,  73,  79,  78,  95,
    88,  0,   80,  79,  83,  73,  84,  73,  79,  78,  95,  89,  0,   80,  79,
    83,  73,  84,  73,  79,  78,  95,  90,  0,   171, 171, 171, 79,  83,  71,
    78,  80,  0,   0,   0,   2,   0,   0,   0,   8,   0,   0,   0,   56,  0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,
    0,   0,   0,   7,   8,   0,   0,   67,  0,   0,   0,   0,   0,   0,   0,
    1,   0,   0,   0,   3,   0,   0,   0,   1,   0,   0,   0,   15,  0,   0,
    0,   80,  79,  83,  73,  84,  73,  79,  78,  79,  83,  0,   83,  86,  95,
    80,  111, 115, 105, 116, 105, 111, 110, 0,   171, 83,  72,  68,  82,  116,
    1,   0,   0,   64,  0,   1,   0,   93,  0,   0,   0,   89,  0,   0,   4,
    70,  142, 32,  0,   0,   0,   0,   0,   6,   0,   0,   0,   95,  0,   0,
    3,   18,  16,  16,  0,   0,   0,   0,   0,   95,  0,   0,   3,   18,  16,
    16,  0,   1,   0,   0,   0,   95,  0,   0,   3,   18,  16,  16,  0,   2,
    0,   0,   0,   101, 0,   0,   3,   114, 32,  16,  0,   0,   0,   0,   0,
    103, 0,   0,   4,   242, 32,  16,  0,   1,   0,   0,   0,   1,   0,   0,
    0,   104, 0,   0,   2,   1,   0,   0,   0,   43,  0,   0,   5,   18,  0,
    16,  0,   0,   0,   0,   0,   10,  16,  16,  0,   0,   0,   0,   0,   43,
    0,   0,   5,   34,  0,   16,  0,   0,   0,   0,   0,   10,  16,  16,  0,
    1,   0,   0,   0,   43,  0,   0,   5,   66,  0,   16,  0,   0,   0,   0,
    0,   10,  16,  16,  0,   2,   0,   0,   0,   50,  0,   0,   11,  114, 0,
    16,  0,   0,   0,   0,   0,   70,  2,   16,  0,   0,   0,   0,   0,   70,
    130, 32,  0,   0,   0,   0,   0,   0,   0,   0,   0,   70,  130, 32,  0,
    0,   0,   0,   0,   1,   0,   0,   0,   54,  0,   0,   5,   114, 32,  16,
    0,   0,   0,   0,   0,   70,  2,   16,  0,   0,   0,   0,   0,   54,  0,
    0,   5,   130, 0,   16,  0,   0,   0,   0,   0,   1,   64,  0,   0,   0,
    0,   128, 63,  17,  0,   0,   8,   18,  32,  16,  0,   1,   0,   0,   0,
    70,  14,  16,  0,   0,   0,   0,   0,   70,  142, 32,  0,   0,   0,   0,
    0,   2,   0,   0,   0,   17,  0,   0,   8,   34,  32,  16,  0,   1,   0,
    0,   0,   70,  14,  16,  0,   0,   0,   0,   0,   70,  142, 32,  0,   0,
    0,   0,   0,   3,   0,   0,   0,   17,  0,   0,   8,   66,  32,  16,  0,
    1,   0,   0,   0,   70,  14,  16,  0,   0,   0,   0,   0,   70,  142, 32,
    0,   0,   0,   0,   0,   4,   0,   0,   0,   17,  0,   0,   8,   130, 32,
    16,  0,   1,   0,   0,   0,   70,  14,  16,  0,   0,   0,   0,   0,   70,
    142, 32,  0,   0,   0,   0,   0,   5,   0,   0,   0,   62,  0,   0,   1,
    83,  84,  65,  84,  116, 0,   0,   0,   11,  0,   0,   0,   1,   0,   0,
    0,   0,   0,   0,   0,   5,   0,   0,   0,   5,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,   0,   0,
    0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0};

const BYTE g_main_ps[] =
   {68,  88,  66,  67,  120, 212, 107, 210, 142, 124, 177, 103, 246, 50,  138,
    163, 137, 24,  83,  32,  1,   0,   0,   0,   48,  3,   0,   0,   5,   0,
    0,   0,   52,  0,   0,   0,   128, 0,   0,   0,   216, 0,   0,   0,   12,
    1,   0,   0,   180, 2,   0,   0,   82,  68,  69,  70,  68,  0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   28,  0,   0,
    0,   0,   4,   255, 255, 0,   1,   0,   0,   28,  0,   0,   0,   77,  105,
    99,  114, 111, 115, 111, 102, 116, 32,  40,  82,  41,  32,  72,  76,  83,
    76,  32,  83,  104, 97,  100, 101, 114, 32,  67,  111, 109, 112, 105, 108,
    101, 114, 32,  49,  48,  46,  49,  0,   73,  83,  71,  78,  80,  0,   0,
    0,   2,   0,   0,   0,   8,   0,   0,   0,   56,  0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   7,
    7,   0,   0,   67,  0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,
    3,   0,   0,   0,   1,   0,   0,   0,   15,  0,   0,   0,   80,  79,  83,
    73,  84,  73,  79,  78,  79,  83,  0,   83,  86,  95,  80,  111, 115, 105,
    116, 105, 111, 110, 0,   171, 79,  83,  71,  78,  44,  0,   0,   0,   1,
    0,   0,   0,   8,   0,   0,   0,   32,  0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   15,  0,   0,
    0,   83,  86,  95,  84,  97,  114, 103, 101, 116, 0,   171, 171, 83,  72,
    68,  82,  160, 1,   0,   0,   64,  0,   0,   0,   104, 0,   0,   0,   98,
    16,  0,   3,   114, 16,  16,  0,   0,   0,   0,   0,   101, 0,   0,   3,
    242, 32,  16,  0,   0,   0,   0,   0,   104, 0,   0,   2,   3,   0,   0,
    0,   11,  0,   0,   5,   114, 0,   16,  0,   0,   0,   0,   0,   38,  25,
    16,  0,   0,   0,   0,   0,   12,  0,   0,   5,   114, 0,   16,  0,   1,
    0,   0,   0,   150, 20,  16,  0,   0,   0,   0,   0,   56,  0,   0,   7,
    114, 0,   16,  0,   2,   0,   0,   0,   70,  2,   16,  0,   0,   0,   0,
    0,   70,  2,   16,  0,   1,   0,   0,   0,   50,  0,   0,   10,  114, 0,
    16,  0,   0,   0,   0,   0,   38,  9,   16,  0,   0,   0,   0,   0,   150,
    4,   16,  0,   1,   0,   0,   0,   70,  2,   16,  128, 65,  0,   0,   0,
    2,   0,   0,   0,   16,  0,   0,   7,   130, 0,   16,  0,   0,   0,   0,
    0,   70,  2,   16,  0,   0,   0,   0,   0,   70,  2,   16,  0,   0,   0,
    0,   0,   68,  0,   0,   5,   130, 0,   16,  0,   0,   0,   0,   0,   58,
    0,   16,  0,   0,   0,   0,   0,   56,  0,   0,   7,   114, 0,   16,  0,
    0,   0,   0,   0,   246, 15,  16,  0,   0,   0,   0,   0,   70,  2,   16,
    0,   0,   0,   0,   0,   16,  32,  0,   10,  18,  0,   16,  0,   0,   0,
    0,   0,   70,  2,   16,  0,   0,   0,   0,   0,   2,   64,  0,   0,   69,
    97,  235, 62,  139, 238, 93,  191, 11,  60,  69,  62,  0,   0,   0,   0,
    75,  0,   0,   5,   18,  0,   16,  0,   0,   0,   0,   0,   10,  0,   16,
    0,   0,   0,   0,   0,   0,   0,   0,   8,   34,  0,   16,  0,   0,   0,
    0,   0,   10,  0,   16,  128, 65,  0,   0,   0,   0,   0,   0,   0,   1,
    64,  0,   0,   0,   0,   128, 63,  56,  0,   0,   7,   34,  0,   16,  0,
    0,   0,   0,   0,   26,  0,   16,  0,   0,   0,   0,   0,   1,   64,  0,
    0,   0,   0,   128, 62,  50,  0,   0,   12,  114, 32,  16,  0,   0,   0,
    0,   0,   6,   0,   16,  0,   0,   0,   0,   0,   2,   64,  0,   0,   154,
    153, 25,  63,  154, 153, 25,  63,  154, 153, 25,  63,  0,   0,   0,   0,
    86,  5,   16,  0,   0,   0,   0,   0,   54,  0,   0,   5,   130, 32,  16,
    0,   0,   0,   0,   0,   1,   64,  0,   0,   0,   0,   128, 63,  62,  0,
    0,   1,   83,  84,  65,  84,  116, 0,   0,   0,   14,  0,   0,   0,   3,
    0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,   12,  0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0};

}

Debug_model_draw::Debug_model_draw(ID3D11Device& device)
   : _device{copy_raw_com_ptr(device)}
{
}

void Debug_model_draw::draw(ID3D11DeviceContext2& dc, const Model& model,
                            const float model_rotation, ID3D11Buffer* index_buffer,
                            ID3D11Buffer* vertex_buffer, const D3D11_VIEWPORT& viewport,
                            ID3D11RenderTargetView* rtv,
                            ID3D11DepthStencilView* dsv) noexcept
{
   if (!_input_layout) initialize();

   dc.ClearState();

   dc.ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
   dc.ClearRenderTargetView(rtv, std::array{0.0f, 0.0f, 0.0f, 1.0f}.data());

   const float bounding_radius = glm::distance(model.bbox.max, model.bbox.min) * 0.5f;
   const float camera_view_size = bounding_radius * 1.25f;
   const float aspect_ratio = viewport.Height / viewport.Width;

   glm::mat4 rotation = glm::identity<glm::mat4>();

   rotation = glm::rotate(rotation, model_rotation, glm::vec3{0.0f, 1.0f, 0.0f});
   // rotation = glm::rotate(rotation, -0.7853982f, glm::vec3{1.0f, 0.0f, 0.0f});

   const Constants constants = {
      .position_decompress_mul = model.position_decompress_mul,
      .position_decompress_add = glm::vec3{0.0f, 0.0f, 0.0f},
      .projection_matrix = glm::transpose(
         glm::orthoRH_ZO(-camera_view_size, camera_view_size,
                         aspect_ratio * -camera_view_size, aspect_ratio * camera_view_size,
                         -camera_view_size * glm::pi<float>(),
                         camera_view_size * glm::pi<float>()) *
         rotation),
   };

   D3D11_MAPPED_SUBRESOURCE mapped = {};

   if (FAILED(dc.Map(_constants.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
      log_and_terminate("Failed to update constant buffer.");
   }

   std::memcpy(mapped.pData, &constants, sizeof(constants));

   dc.Unmap(_constants.get(), 0);

   ID3D11Buffer* constant_buffer = _constants.get();

   dc.VSSetConstantBuffers(0, 1, &constant_buffer);

   dc.IASetInputLayout(_input_layout.get());
   dc.IASetIndexBuffer(index_buffer, DXGI_FORMAT_R16_UINT, 0);

   dc.VSSetShader(_vertex_shader.get(), nullptr, 0);

   dc.RSSetViewports(1, &viewport);
   dc.RSSetState(_rasterizer_state.get());

   dc.PSSetShader(_pixel_shader.get(), nullptr, 0);

   dc.OMSetRenderTargets(1, &rtv, dsv);
   dc.OMSetBlendState(nullptr, nullptr, 0xff'ff'ff'ffu);
   dc.OMSetDepthStencilState(_depth_stencil_state.get(), 0xffu);

   const UINT untextured_stride = 6;
   const UINT offset = 0;

   dc.IASetVertexBuffers(0, 1, &vertex_buffer, &untextured_stride, &offset);

   for (const Model_segment& segment : model.opaque_segments) {
      dc.IASetPrimitiveTopology(segment.topology);
      dc.DrawIndexed(segment.index_count, segment.start_index, segment.base_vertex);
   }

   dc.RSSetState(_doublesided_rasterizer_state.get());

   for (const Model_segment& segment : model.doublesided_segments) {
      dc.IASetPrimitiveTopology(segment.topology);
      dc.DrawIndexed(segment.index_count, segment.start_index, segment.base_vertex);
   }

   const UINT textured_stride = 10;

   dc.IASetVertexBuffers(0, 1, &vertex_buffer, &textured_stride, &offset);
   dc.RSSetState(_rasterizer_state.get());

   for (const Model_segment_hardedged& segment : model.hardedged_segments) {
      dc.IASetPrimitiveTopology(segment.topology);
      dc.DrawIndexed(segment.index_count, segment.start_index, segment.base_vertex);
   }

   dc.RSSetState(_doublesided_rasterizer_state.get());

   for (const Model_segment_hardedged& segment : model.hardedged_doublesided_segments) {
      dc.IASetPrimitiveTopology(segment.topology);
      dc.DrawIndexed(segment.index_count, segment.start_index, segment.base_vertex);
   }
}

void Debug_model_draw::initialize() noexcept
{
   const D3D11_BUFFER_DESC constant_buffer_desc = {
      .ByteWidth = sizeof(Constants),
      .Usage = D3D11_USAGE_DYNAMIC,
      .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
      .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
   };

   if (FAILED(_device->CreateBuffer(&constant_buffer_desc, nullptr,
                                    _constants.clear_and_assign()))) {
      log_and_terminate("Failed to create debug model draw resource.");
   }

   const std::array<D3D11_INPUT_ELEMENT_DESC, 3> input_layout = {
      D3D11_INPUT_ELEMENT_DESC{
         .SemanticName = "POSITION_X",
         .Format = DXGI_FORMAT_R16_SINT,
         .AlignedByteOffset = 0,
      },
      D3D11_INPUT_ELEMENT_DESC{
         .SemanticName = "POSITION_Y",
         .Format = DXGI_FORMAT_R16_SINT,
         .AlignedByteOffset = 2,
      },
      D3D11_INPUT_ELEMENT_DESC{
         .SemanticName = "POSITION_Z",
         .Format = DXGI_FORMAT_R16_SINT,
         .AlignedByteOffset = 4,
      },
   };

   if (FAILED(_device->CreateInputLayout(input_layout.data(), input_layout.size(),
                                         &g_main_vs[0], sizeof(g_main_vs),
                                         _input_layout.clear_and_assign()))) {
      log_and_terminate("Failed to create debug model draw resource.");
   }

   if (FAILED(_device->CreateVertexShader(&g_main_vs[0], sizeof(g_main_vs), nullptr,
                                          _vertex_shader.clear_and_assign()))) {
      log_and_terminate("Failed to create debug model draw resource.");
   }

   D3D11_RASTERIZER_DESC rasterizer_desc = {
      .FillMode = D3D11_FILL_SOLID,
      .CullMode = D3D11_CULL_BACK,
      .FrontCounterClockwise = true,
      .DepthClipEnable = true,
   };

   if (FAILED(_device->CreateRasterizerState(&rasterizer_desc,
                                             _rasterizer_state.clear_and_assign()))) {
      log_and_terminate("Failed to create debug model draw resource.");
   }

   rasterizer_desc.CullMode = D3D11_CULL_NONE;

   if (FAILED(_device->CreateRasterizerState(&rasterizer_desc,
                                             _doublesided_rasterizer_state.clear_and_assign()))) {
      log_and_terminate("Failed to create debug model draw resource.");
   }

   const D3D11_DEPTH_STENCIL_DESC depth_stencil_desc = {
      .DepthEnable = true,
      .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
      .DepthFunc = D3D11_COMPARISON_LESS_EQUAL,
   };

   if (FAILED(_device->CreateDepthStencilState(&depth_stencil_desc,
                                               _depth_stencil_state.clear_and_assign()))) {
      log_and_terminate("Failed to create debug model draw resource.");
   }

   if (FAILED(_device->CreatePixelShader(&g_main_ps[0], sizeof(g_main_ps), nullptr,
                                         _pixel_shader.clear_and_assign()))) {
      log_and_terminate("Failed to create debug model draw resource.");
   }
}

}