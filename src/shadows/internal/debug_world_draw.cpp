#include "debug_world_draw.hpp"

#include "../../imgui/imgui.h"
#include "../../logger.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <numbers>

namespace sp::shadows {

namespace {

void push_info_filter(ID3D11Device& device)
{
   Com_ptr<ID3D11Debug> debug;

   if (auto result = device.QueryInterface(debug.clear_and_assign());
       SUCCEEDED(result)) {

      Com_ptr<ID3D11InfoQueue> infoqueue;
      if (result = debug->QueryInterface(infoqueue.clear_and_assign());
          SUCCEEDED(result)) {
         std::array hide{D3D11_MESSAGE_ID_DEVICE_DRAW_RENDERTARGETVIEW_NOT_SET};

         D3D11_INFO_QUEUE_FILTER filter{};
         filter.DenyList.NumIDs = hide.size();
         filter.DenyList.pIDList = hide.data();

         infoqueue->PushStorageFilter(&filter);
      }
   }
}

void pop_info_filter(ID3D11Device& device)
{
   Com_ptr<ID3D11Debug> debug;

   if (auto result = device.QueryInterface(debug.clear_and_assign());
       SUCCEEDED(result)) {

      Com_ptr<ID3D11InfoQueue> infoqueue;
      if (result = debug->QueryInterface(infoqueue.clear_and_assign());
          SUCCEEDED(result)) {
         infoqueue->PopStorageFilter();
      }
   }
}

auto sample_color(std::uint32_t i) noexcept -> glm::vec3
{
   // Martin Roberts - The Unreasonable Effectiveness of Quasirandom Sequences https://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences/

   constexpr double g = 1.22074408460575947536;
   constexpr double a1 = 1.0 / g;
   constexpr double a2 = 1.0 / (g * g);
   constexpr double a3 = 1.0 / (g * g * g);

   const glm::vec3 samples =
      glm::fract(glm::dvec3{a1 * i, a2 * i, a3 * i}) * glm::dvec3{1.0, 0.5, 0.5} +
      glm::dvec3{0.0, 0.5, 0.5};

   glm::vec3 color;

   ImGui::ColorConvertHSVtoRGB(samples.x, samples.y, samples.z, color.r,
                               color.g, color.b);

   return color;
}

struct alignas(16) Constants {
   glm::vec3 position_decompress_mul;
   std::uint32_t pad0;

   glm::vec3 position_decompress_add;
   std::uint32_t pad1;

   std::array<glm::vec4, 3> rotation_matrix;
   glm::vec3 translateWS;
   std::uint32_t pad2;

   glm::mat4 projection_matrix;

   glm::vec3 color;
   std::uint32_t object_index;
};

// Shader for below bytecode
// Compiled with:
// VS: fxc /T vs_4_0 /E main_vs /Fh shader.h <shader>.hlsl
// PS: fxc /T ps_4_0 /E main_ps /Fh shader.h <shader>.hlsl
#if 0

cbuffer Constants {
   float3 position_decompress_mul;
   float3 position_decompress_add;
   
   float3x3 rotation_matrix;
   float3 translateWS;

   float4x4 projection_matrix;

   float3 color;
   uint object_index;
};

struct Vertex_output {
   float3 position_offsetWS : POSITIONWS;
   float4 positionPS : SV_Position;
};

struct Pixel_output {
   float4 color : SV_Target0;
   uint object_index : SV_Target1;
};

Vertex_output main_vs(int position_XCS : POSITION_X, int position_YCS : POSITION_Y, int position_ZCS : POSITION_Z)
{
   int3 positionCS = {position_XCS, position_YCS, position_ZCS};
   float3 positionOS = positionCS.xyz * position_decompress_mul + position_decompress_add;
   float3 position_almostWS = mul(positionOS, rotation_matrix);
   float3 positionWS = position_almostWS + translateWS;

   Vertex_output output;

   output.position_offsetWS = position_almostWS;
   output.positionPS = mul(float4(positionWS, 1.0), projection_matrix);

   return output;
}

Pixel_output main_ps(Vertex_output input) 
{   
   const float3 light_normalWS = normalize(float3(159.264923, -300.331013, 66.727310));
   float3 normalWS = normalize(cross(ddx(input.position_offsetWS), ddy(input.position_offsetWS)));

   float light = sqrt(saturate(dot(normalize(normalWS), light_normalWS)));

   light = light * 0.6 + (1.0 - light) * 0.25;

   Pixel_output output;

   output.color = float4(light * color, 1.0f);
   output.object_index = object_index;

   return output;
}

#endif

const BYTE g_main_vs[] =
   {68,  88,  66,  67,  200, 236, 55,  4,   150, 244, 156, 114, 177, 4,   151,
    65,  198, 172, 140, 31,  1,   0,   0,   0,   108, 5,   0,   0,   5,   0,
    0,   0,   52,  0,   0,   0,   32,  2,   0,   0,   156, 2,   0,   0,   244,
    2,   0,   0,   240, 4,   0,   0,   82,  68,  69,  70,  228, 1,   0,   0,
    1,   0,   0,   0,   72,  0,   0,   0,   1,   0,   0,   0,   28,  0,   0,
    0,   0,   4,   254, 255, 0,   1,   0,   0,   188, 1,   0,   0,   60,  0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,
    67,  111, 110, 115, 116, 97,  110, 116, 115, 0,   171, 171, 60,  0,   0,
    0,   7,   0,   0,   0,   96,  0,   0,   0,   176, 0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   8,   1,   0,   0,   0,   0,   0,   0,   12,
    0,   0,   0,   2,   0,   0,   0,   32,  1,   0,   0,   0,   0,   0,   0,
    48,  1,   0,   0,   16,  0,   0,   0,   12,  0,   0,   0,   2,   0,   0,
    0,   32,  1,   0,   0,   0,   0,   0,   0,   72,  1,   0,   0,   32,  0,
    0,   0,   44,  0,   0,   0,   2,   0,   0,   0,   88,  1,   0,   0,   0,
    0,   0,   0,   104, 1,   0,   0,   80,  0,   0,   0,   12,  0,   0,   0,
    2,   0,   0,   0,   32,  1,   0,   0,   0,   0,   0,   0,   116, 1,   0,
    0,   96,  0,   0,   0,   64,  0,   0,   0,   2,   0,   0,   0,   136, 1,
    0,   0,   0,   0,   0,   0,   152, 1,   0,   0,   160, 0,   0,   0,   12,
    0,   0,   0,   0,   0,   0,   0,   32,  1,   0,   0,   0,   0,   0,   0,
    158, 1,   0,   0,   172, 0,   0,   0,   4,   0,   0,   0,   0,   0,   0,
    0,   172, 1,   0,   0,   0,   0,   0,   0,   112, 111, 115, 105, 116, 105,
    111, 110, 95,  100, 101, 99,  111, 109, 112, 114, 101, 115, 115, 95,  109,
    117, 108, 0,   1,   0,   3,   0,   1,   0,   3,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   112, 111, 115, 105, 116, 105, 111, 110, 95,  100, 101,
    99,  111, 109, 112, 114, 101, 115, 115, 95,  97,  100, 100, 0,   114, 111,
    116, 97,  116, 105, 111, 110, 95,  109, 97,  116, 114, 105, 120, 0,   3,
    0,   3,   0,   3,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    116, 114, 97,  110, 115, 108, 97,  116, 101, 87,  83,  0,   112, 114, 111,
    106, 101, 99,  116, 105, 111, 110, 95,  109, 97,  116, 114, 105, 120, 0,
    171, 171, 3,   0,   3,   0,   4,   0,   4,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   99,  111, 108, 111, 114, 0,   111, 98,  106, 101, 99,  116,
    95,  105, 110, 100, 101, 120, 0,   171, 0,   0,   19,  0,   1,   0,   1,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   77,  105, 99,  114, 111, 115,
    111, 102, 116, 32,  40,  82,  41,  32,  72,  76,  83,  76,  32,  83,  104,
    97,  100, 101, 114, 32,  67,  111, 109, 112, 105, 108, 101, 114, 32,  49,
    48,  46,  49,  0,   73,  83,  71,  78,  116, 0,   0,   0,   3,   0,   0,
    0,   8,   0,   0,   0,   80,  0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   2,   0,   0,   0,   0,   0,   0,   0,   1,   1,   0,   0,   91,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,
    1,   0,   0,   0,   1,   1,   0,   0,   102, 0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   2,   0,   0,   0,   2,   0,   0,   0,   1,   1,
    0,   0,   80,  79,  83,  73,  84,  73,  79,  78,  95,  88,  0,   80,  79,
    83,  73,  84,  73,  79,  78,  95,  89,  0,   80,  79,  83,  73,  84,  73,
    79,  78,  95,  90,  0,   171, 171, 171, 79,  83,  71,  78,  80,  0,   0,
    0,   2,   0,   0,   0,   8,   0,   0,   0,   56,  0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   7,
    8,   0,   0,   67,  0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,
    3,   0,   0,   0,   1,   0,   0,   0,   15,  0,   0,   0,   80,  79,  83,
    73,  84,  73,  79,  78,  87,  83,  0,   83,  86,  95,  80,  111, 115, 105,
    116, 105, 111, 110, 0,   171, 83,  72,  68,  82,  244, 1,   0,   0,   64,
    0,   1,   0,   125, 0,   0,   0,   89,  0,   0,   4,   70,  142, 32,  0,
    0,   0,   0,   0,   10,  0,   0,   0,   95,  0,   0,   3,   18,  16,  16,
    0,   0,   0,   0,   0,   95,  0,   0,   3,   18,  16,  16,  0,   1,   0,
    0,   0,   95,  0,   0,   3,   18,  16,  16,  0,   2,   0,   0,   0,   101,
    0,   0,   3,   114, 32,  16,  0,   0,   0,   0,   0,   103, 0,   0,   4,
    242, 32,  16,  0,   1,   0,   0,   0,   1,   0,   0,   0,   104, 0,   0,
    2,   2,   0,   0,   0,   43,  0,   0,   5,   18,  0,   16,  0,   0,   0,
    0,   0,   10,  16,  16,  0,   0,   0,   0,   0,   43,  0,   0,   5,   34,
    0,   16,  0,   0,   0,   0,   0,   10,  16,  16,  0,   1,   0,   0,   0,
    43,  0,   0,   5,   66,  0,   16,  0,   0,   0,   0,   0,   10,  16,  16,
    0,   2,   0,   0,   0,   50,  0,   0,   11,  114, 0,   16,  0,   0,   0,
    0,   0,   70,  2,   16,  0,   0,   0,   0,   0,   70,  130, 32,  0,   0,
    0,   0,   0,   0,   0,   0,   0,   70,  130, 32,  0,   0,   0,   0,   0,
    1,   0,   0,   0,   16,  0,   0,   8,   18,  0,   16,  0,   1,   0,   0,
    0,   70,  2,   16,  0,   0,   0,   0,   0,   70,  130, 32,  0,   0,   0,
    0,   0,   2,   0,   0,   0,   16,  0,   0,   8,   34,  0,   16,  0,   1,
    0,   0,   0,   70,  2,   16,  0,   0,   0,   0,   0,   70,  130, 32,  0,
    0,   0,   0,   0,   3,   0,   0,   0,   16,  0,   0,   8,   66,  0,   16,
    0,   1,   0,   0,   0,   70,  2,   16,  0,   0,   0,   0,   0,   70,  130,
    32,  0,   0,   0,   0,   0,   4,   0,   0,   0,   54,  0,   0,   5,   114,
    32,  16,  0,   0,   0,   0,   0,   70,  2,   16,  0,   1,   0,   0,   0,
    0,   0,   0,   8,   114, 0,   16,  0,   0,   0,   0,   0,   70,  2,   16,
    0,   1,   0,   0,   0,   70,  130, 32,  0,   0,   0,   0,   0,   5,   0,
    0,   0,   54,  0,   0,   5,   130, 0,   16,  0,   0,   0,   0,   0,   1,
    64,  0,   0,   0,   0,   128, 63,  17,  0,   0,   8,   18,  32,  16,  0,
    1,   0,   0,   0,   70,  14,  16,  0,   0,   0,   0,   0,   70,  142, 32,
    0,   0,   0,   0,   0,   6,   0,   0,   0,   17,  0,   0,   8,   34,  32,
    16,  0,   1,   0,   0,   0,   70,  14,  16,  0,   0,   0,   0,   0,   70,
    142, 32,  0,   0,   0,   0,   0,   7,   0,   0,   0,   17,  0,   0,   8,
    66,  32,  16,  0,   1,   0,   0,   0,   70,  14,  16,  0,   0,   0,   0,
    0,   70,  142, 32,  0,   0,   0,   0,   0,   8,   0,   0,   0,   17,  0,
    0,   8,   130, 32,  16,  0,   1,   0,   0,   0,   70,  14,  16,  0,   0,
    0,   0,   0,   70,  142, 32,  0,   0,   0,   0,   0,   9,   0,   0,   0,
    62,  0,   0,   1,   83,  84,  65,  84,  116, 0,   0,   0,   15,  0,   0,
    0,   2,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   0,   9,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   0,
    0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0};

const BYTE g_main_ps[] =
   {68,  88,  66,  67,  215, 147, 2,   126, 67,  203, 49,  1,   220, 173, 239,
    249, 80,  176, 14,  1,   1,   0,   0,   0,   48,  5,   0,   0,   5,   0,
    0,   0,   52,  0,   0,   0,   32,  2,   0,   0,   120, 2,   0,   0,   196,
    2,   0,   0,   180, 4,   0,   0,   82,  68,  69,  70,  228, 1,   0,   0,
    1,   0,   0,   0,   72,  0,   0,   0,   1,   0,   0,   0,   28,  0,   0,
    0,   0,   4,   255, 255, 0,   1,   0,   0,   188, 1,   0,   0,   60,  0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,
    67,  111, 110, 115, 116, 97,  110, 116, 115, 0,   171, 171, 60,  0,   0,
    0,   7,   0,   0,   0,   96,  0,   0,   0,   176, 0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   8,   1,   0,   0,   0,   0,   0,   0,   12,
    0,   0,   0,   0,   0,   0,   0,   32,  1,   0,   0,   0,   0,   0,   0,
    48,  1,   0,   0,   16,  0,   0,   0,   12,  0,   0,   0,   0,   0,   0,
    0,   32,  1,   0,   0,   0,   0,   0,   0,   72,  1,   0,   0,   32,  0,
    0,   0,   44,  0,   0,   0,   0,   0,   0,   0,   88,  1,   0,   0,   0,
    0,   0,   0,   104, 1,   0,   0,   80,  0,   0,   0,   12,  0,   0,   0,
    0,   0,   0,   0,   32,  1,   0,   0,   0,   0,   0,   0,   116, 1,   0,
    0,   96,  0,   0,   0,   64,  0,   0,   0,   0,   0,   0,   0,   136, 1,
    0,   0,   0,   0,   0,   0,   152, 1,   0,   0,   160, 0,   0,   0,   12,
    0,   0,   0,   2,   0,   0,   0,   32,  1,   0,   0,   0,   0,   0,   0,
    158, 1,   0,   0,   172, 0,   0,   0,   4,   0,   0,   0,   2,   0,   0,
    0,   172, 1,   0,   0,   0,   0,   0,   0,   112, 111, 115, 105, 116, 105,
    111, 110, 95,  100, 101, 99,  111, 109, 112, 114, 101, 115, 115, 95,  109,
    117, 108, 0,   1,   0,   3,   0,   1,   0,   3,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   112, 111, 115, 105, 116, 105, 111, 110, 95,  100, 101,
    99,  111, 109, 112, 114, 101, 115, 115, 95,  97,  100, 100, 0,   114, 111,
    116, 97,  116, 105, 111, 110, 95,  109, 97,  116, 114, 105, 120, 0,   3,
    0,   3,   0,   3,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    116, 114, 97,  110, 115, 108, 97,  116, 101, 87,  83,  0,   112, 114, 111,
    106, 101, 99,  116, 105, 111, 110, 95,  109, 97,  116, 114, 105, 120, 0,
    171, 171, 3,   0,   3,   0,   4,   0,   4,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   99,  111, 108, 111, 114, 0,   111, 98,  106, 101, 99,  116,
    95,  105, 110, 100, 101, 120, 0,   171, 0,   0,   19,  0,   1,   0,   1,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   77,  105, 99,  114, 111, 115,
    111, 102, 116, 32,  40,  82,  41,  32,  72,  76,  83,  76,  32,  83,  104,
    97,  100, 101, 114, 32,  67,  111, 109, 112, 105, 108, 101, 114, 32,  49,
    48,  46,  49,  0,   73,  83,  71,  78,  80,  0,   0,   0,   2,   0,   0,
    0,   8,   0,   0,   0,   56,  0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   7,   7,   0,   0,   67,
    0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   3,   0,   0,   0,
    1,   0,   0,   0,   15,  0,   0,   0,   80,  79,  83,  73,  84,  73,  79,
    78,  87,  83,  0,   83,  86,  95,  80,  111, 115, 105, 116, 105, 111, 110,
    0,   171, 79,  83,  71,  78,  68,  0,   0,   0,   2,   0,   0,   0,   8,
    0,   0,   0,   56,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    3,   0,   0,   0,   0,   0,   0,   0,   15,  0,   0,   0,   56,  0,   0,
    0,   1,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   1,   0,
    0,   0,   1,   14,  0,   0,   83,  86,  95,  84,  97,  114, 103, 101, 116,
    0,   171, 171, 83,  72,  68,  82,  232, 1,   0,   0,   64,  0,   0,   0,
    122, 0,   0,   0,   89,  0,   0,   4,   70,  142, 32,  0,   0,   0,   0,
    0,   11,  0,   0,   0,   98,  16,  0,   3,   114, 16,  16,  0,   0,   0,
    0,   0,   101, 0,   0,   3,   242, 32,  16,  0,   0,   0,   0,   0,   101,
    0,   0,   3,   18,  32,  16,  0,   1,   0,   0,   0,   104, 0,   0,   2,
    3,   0,   0,   0,   11,  0,   0,   5,   114, 0,   16,  0,   0,   0,   0,
    0,   38,  25,  16,  0,   0,   0,   0,   0,   12,  0,   0,   5,   114, 0,
    16,  0,   1,   0,   0,   0,   150, 20,  16,  0,   0,   0,   0,   0,   56,
    0,   0,   7,   114, 0,   16,  0,   2,   0,   0,   0,   70,  2,   16,  0,
    0,   0,   0,   0,   70,  2,   16,  0,   1,   0,   0,   0,   50,  0,   0,
    10,  114, 0,   16,  0,   0,   0,   0,   0,   38,  9,   16,  0,   0,   0,
    0,   0,   150, 4,   16,  0,   1,   0,   0,   0,   70,  2,   16,  128, 65,
    0,   0,   0,   2,   0,   0,   0,   16,  0,   0,   7,   130, 0,   16,  0,
    0,   0,   0,   0,   70,  2,   16,  0,   0,   0,   0,   0,   70,  2,   16,
    0,   0,   0,   0,   0,   68,  0,   0,   5,   130, 0,   16,  0,   0,   0,
    0,   0,   58,  0,   16,  0,   0,   0,   0,   0,   56,  0,   0,   7,   114,
    0,   16,  0,   0,   0,   0,   0,   246, 15,  16,  0,   0,   0,   0,   0,
    70,  2,   16,  0,   0,   0,   0,   0,   16,  32,  0,   10,  18,  0,   16,
    0,   0,   0,   0,   0,   70,  2,   16,  0,   0,   0,   0,   0,   2,   64,
    0,   0,   69,  97,  235, 62,  139, 238, 93,  191, 11,  60,  69,  62,  0,
    0,   0,   0,   75,  0,   0,   5,   18,  0,   16,  0,   0,   0,   0,   0,
    10,  0,   16,  0,   0,   0,   0,   0,   0,   0,   0,   8,   34,  0,   16,
    0,   0,   0,   0,   0,   10,  0,   16,  128, 65,  0,   0,   0,   0,   0,
    0,   0,   1,   64,  0,   0,   0,   0,   128, 63,  56,  0,   0,   7,   34,
    0,   16,  0,   0,   0,   0,   0,   26,  0,   16,  0,   0,   0,   0,   0,
    1,   64,  0,   0,   0,   0,   128, 62,  50,  0,   0,   9,   18,  0,   16,
    0,   0,   0,   0,   0,   10,  0,   16,  0,   0,   0,   0,   0,   1,   64,
    0,   0,   154, 153, 25,  63,  26,  0,   16,  0,   0,   0,   0,   0,   56,
    0,   0,   8,   114, 32,  16,  0,   0,   0,   0,   0,   6,   0,   16,  0,
    0,   0,   0,   0,   70,  130, 32,  0,   0,   0,   0,   0,   10,  0,   0,
    0,   54,  0,   0,   5,   130, 32,  16,  0,   0,   0,   0,   0,   1,   64,
    0,   0,   0,   0,   128, 63,  54,  0,   0,   6,   18,  32,  16,  0,   1,
    0,   0,   0,   58,  128, 32,  0,   0,   0,   0,   0,   10,  0,   0,   0,
    62,  0,   0,   1,   83,  84,  65,  84,  116, 0,   0,   0,   16,  0,   0,
    0,   3,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   13,  0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0};

}

Debug_world_draw::Debug_world_draw(ID3D11Device& device)
   : _device{copy_raw_com_ptr(device)}
{
}

void Debug_world_draw::draw(ID3D11DeviceContext2& dc, const World_inputs& world,
                            const glm::mat4& projection_matrix,
                            ID3D11Buffer* index_buffer, ID3D11Buffer* vertex_buffer,
                            const Target_inputs& target) noexcept
{
   if (!_input_layout) initialize();

   push_info_filter(*_device);

   dc.ClearState();

   dc.ClearDepthStencilView(target.dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
   if (target.rtv)
      dc.ClearRenderTargetView(target.rtv, std::array{0.0f, 0.0f, 0.0f, 1.0f}.data());
   if (target.picking_rtv)
      dc.ClearRenderTargetView(target.picking_rtv,
                               std::array{4294967295.0f, 0.0f, 0.0f, 0.0f}.data());

   dc.IASetInputLayout(_input_layout.get());
   dc.IASetIndexBuffer(index_buffer, DXGI_FORMAT_R16_UINT, 0);

   dc.VSSetShader(_vertex_shader.get(), nullptr, 0);

   dc.RSSetViewports(1, &target.viewport);

   dc.PSSetShader(_pixel_shader.get(), nullptr, 0);

   std::array<ID3D11RenderTargetView*, 2> rtvs = {target.rtv, target.picking_rtv};

   dc.OMSetRenderTargets(rtvs.size(), rtvs.data(), target.dsv);
   dc.OMSetBlendState(nullptr, nullptr, 0xff'ff'ff'ffu);
   dc.OMSetDepthStencilState(_depth_stencil_state.get(), 0xffu);

   for (std::uint32_t i = 0; i < world.object_instances.size(); ++i) {
      const Object_instance& object = world.object_instances[i];
      const Game_model& game_model = world.game_models[object.game_model_index];
      const Model& model = world.models[game_model.lod0_index];

      const Constants constants =
         {.position_decompress_mul =
             (model.bbox_max - model.bbox_min) * (0.5f / INT16_MAX),
          .position_decompress_add = (model.bbox_max + model.bbox_min) * 0.5f,
          .rotation_matrix = {glm::vec4{object.rotation[0], 0.0f},
                              glm::vec4{object.rotation[1], 0.0f},
                              glm::vec4{object.rotation[2], 0.0f}},
          .translateWS = object.positionWS,
          .projection_matrix = projection_matrix,
          .color = sample_color(i),
          .object_index = i};

      D3D11_MAPPED_SUBRESOURCE mapped = {};

      if (FAILED(dc.Map(_constants.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
         log_and_terminate("Failed to update constant buffer.");
      }

      std::memcpy(mapped.pData, &constants, sizeof(constants));

      dc.Unmap(_constants.get(), 0);

      ID3D11Buffer* constant_buffer = _constants.get();

      dc.VSSetConstantBuffers(0, 1, &constant_buffer);
      dc.PSSetConstantBuffers(0, 1, &constant_buffer);

      const UINT untextured_stride = 6;
      const UINT offset = 0;

      dc.IASetVertexBuffers(0, 1, &vertex_buffer, &untextured_stride, &offset);
      dc.RSSetState(_rasterizer_state.get());

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

   pop_info_filter(*_device);
}

void Debug_world_draw::draw(ID3D11DeviceContext2& dc, const World_inputs& world,
                            const float camera_yaw, const float camera_pitch,
                            const glm::vec3& camera_positioWS,
                            ID3D11Buffer* index_buffer, ID3D11Buffer* vertex_buffer,
                            const Target_inputs& target) noexcept
{
   glm::mat4 view_matrix = glm::identity<glm::mat4>();

   view_matrix = glm::rotate(view_matrix, camera_pitch, glm::vec3{1.0f, 0.0f, 0.0f});
   view_matrix = glm::rotate(view_matrix, camera_yaw, glm::vec3{0.0f, 1.0f, 0.0f});
   view_matrix = glm::translate(view_matrix, camera_positioWS);

   const glm::mat4 projection_matrix = glm::transpose(
      glm::perspectiveFovRH_ZO(1.5707964f, static_cast<float>(target.viewport.Width),
                               static_cast<float>(target.viewport.Height), 1.0f, 2048.0f) *
      view_matrix);

   draw(dc, world, projection_matrix, index_buffer, vertex_buffer, target);
}

void Debug_world_draw::initialize() noexcept
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