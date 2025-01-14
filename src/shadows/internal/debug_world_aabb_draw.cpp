#include "debug_world_aabb_draw.hpp"

#include "../../imgui/imgui.h"
#include "../../logger.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <numbers>

namespace sp::shadows {

namespace {

auto sample_color(std::uint32_t i) noexcept -> glm::vec3
{
   // Martin Roberts - The Unreasonable Effectiveness of Quasirandom Sequences https://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences/

   constexpr double g = 1.22074408460575947536;
   constexpr double a1 = 1.0 / g;
   constexpr double a2 = 1.0 / (g * g);
   constexpr double a3 = 1.0 / (g * g * g);

   const glm::vec3 samples =
      glm::fract(glm::dvec3{a1 * i, a2 * i, a3 * i}) * glm::dvec3{1.0, 0.5, 0.25} +
      glm::dvec3{0.0, 0.5, 0.75};

   glm::vec3 color;

   ImGui::ColorConvertHSVtoRGB(samples.x, samples.y, samples.z, color.r,
                               color.g, color.b);

   return color;
}

struct alignas(16) Constants {
   glm::vec3 position_minWS;
   std::uint32_t pad0;

   glm::vec3 position_maxWS;
   std::uint32_t pad1;

   glm::mat4 projection_matrix;

   glm::vec3 color;
   std::uint32_t pad3;
};

// Shader for below bytecode
// Compiled with:
// VS: fxc /T vs_4_0 /E main_vs /Fh shader.h <shader>.hlsl
// PS: fxc /T ps_4_0 /E main_ps /Fh shader.h <shader>.hlsl
#if 0

cbuffer Constants {
   float3 position_minWS;
   float3 position_maxWS;

   float4x4 projection_matrix;

   float3 color;
};

struct Vertex_output {
   float3 normalWS : NORMALWS;
   float4 positionPS : SV_Position;
};

Vertex_output main_vs(float3 positionOS : POSITION, float3 normalWS : NORMAL)
{
   float3 positionWS = lerp(position_minWS, position_maxWS, positionOS);

   Vertex_output output;

   output.normalWS = normalWS;
   output.positionPS = mul(float4(positionWS, 1.0), projection_matrix);

   return output;
}

float4 main_ps(Vertex_output input) : SV_Target0
{   
   const float3 light_normalWS = normalize(float3(159.264923, -300.331013, 66.727310));

   float light = sqrt(saturate(dot(normalize(input.normalWS), light_normalWS)));

   light = light * 0.6 + (1.0 - light) * 0.25;

   return float4(light * color, 1.0f);
}

#endif

const BYTE g_main_vs[] =
   {68,  88,  66,  67,  213, 30,  170, 65,  177, 72,  78,  145, 170, 86,  177,
    43,  137, 174, 118, 79,  1,   0,   0,   0,   252, 3,   0,   0,   5,   0,
    0,   0,   52,  0,   0,   0,   128, 1,   0,   0,   208, 1,   0,   0,   40,
    2,   0,   0,   128, 3,   0,   0,   82,  68,  69,  70,  68,  1,   0,   0,
    1,   0,   0,   0,   72,  0,   0,   0,   1,   0,   0,   0,   28,  0,   0,
    0,   0,   4,   254, 255, 0,   1,   0,   0,   26,  1,   0,   0,   60,  0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,
    67,  111, 110, 115, 116, 97,  110, 116, 115, 0,   171, 171, 60,  0,   0,
    0,   4,   0,   0,   0,   96,  0,   0,   0,   112, 0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   192, 0,   0,   0,   0,   0,   0,   0,   12,
    0,   0,   0,   2,   0,   0,   0,   208, 0,   0,   0,   0,   0,   0,   0,
    224, 0,   0,   0,   16,  0,   0,   0,   12,  0,   0,   0,   2,   0,   0,
    0,   208, 0,   0,   0,   0,   0,   0,   0,   239, 0,   0,   0,   32,  0,
    0,   0,   64,  0,   0,   0,   2,   0,   0,   0,   4,   1,   0,   0,   0,
    0,   0,   0,   20,  1,   0,   0,   96,  0,   0,   0,   12,  0,   0,   0,
    0,   0,   0,   0,   208, 0,   0,   0,   0,   0,   0,   0,   112, 111, 115,
    105, 116, 105, 111, 110, 95,  109, 105, 110, 87,  83,  0,   171, 1,   0,
    3,   0,   1,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   112,
    111, 115, 105, 116, 105, 111, 110, 95,  109, 97,  120, 87,  83,  0,   112,
    114, 111, 106, 101, 99,  116, 105, 111, 110, 95,  109, 97,  116, 114, 105,
    120, 0,   171, 171, 171, 3,   0,   3,   0,   4,   0,   4,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   99,  111, 108, 111, 114, 0,   77,  105, 99,
    114, 111, 115, 111, 102, 116, 32,  40,  82,  41,  32,  72,  76,  83,  76,
    32,  83,  104, 97,  100, 101, 114, 32,  67,  111, 109, 112, 105, 108, 101,
    114, 32,  49,  48,  46,  49,  0,   171, 171, 73,  83,  71,  78,  72,  0,
    0,   0,   2,   0,   0,   0,   8,   0,   0,   0,   56,  0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,
    7,   7,   0,   0,   65,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   3,   0,   0,   0,   1,   0,   0,   0,   7,   7,   0,   0,   80,  79,
    83,  73,  84,  73,  79,  78,  0,   78,  79,  82,  77,  65,  76,  0,   79,
    83,  71,  78,  80,  0,   0,   0,   2,   0,   0,   0,   8,   0,   0,   0,
    56,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,
    0,   0,   0,   0,   0,   7,   8,   0,   0,   65,  0,   0,   0,   0,   0,
    0,   0,   1,   0,   0,   0,   3,   0,   0,   0,   1,   0,   0,   0,   15,
    0,   0,   0,   78,  79,  82,  77,  65,  76,  87,  83,  0,   83,  86,  95,
    80,  111, 115, 105, 116, 105, 111, 110, 0,   171, 171, 171, 83,  72,  68,
    82,  80,  1,   0,   0,   64,  0,   1,   0,   84,  0,   0,   0,   89,  0,
    0,   4,   70,  142, 32,  0,   0,   0,   0,   0,   6,   0,   0,   0,   95,
    0,   0,   3,   114, 16,  16,  0,   0,   0,   0,   0,   95,  0,   0,   3,
    114, 16,  16,  0,   1,   0,   0,   0,   101, 0,   0,   3,   114, 32,  16,
    0,   0,   0,   0,   0,   103, 0,   0,   4,   242, 32,  16,  0,   1,   0,
    0,   0,   1,   0,   0,   0,   104, 0,   0,   2,   1,   0,   0,   0,   54,
    0,   0,   5,   114, 32,  16,  0,   0,   0,   0,   0,   70,  18,  16,  0,
    1,   0,   0,   0,   0,   0,   0,   10,  114, 0,   16,  0,   0,   0,   0,
    0,   70,  130, 32,  128, 65,  0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   70,  130, 32,  0,   0,   0,   0,   0,   1,   0,   0,   0,   50,
    0,   0,   10,  114, 0,   16,  0,   0,   0,   0,   0,   70,  18,  16,  0,
    0,   0,   0,   0,   70,  2,   16,  0,   0,   0,   0,   0,   70,  130, 32,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   54,  0,   0,   5,   130, 0,
    16,  0,   0,   0,   0,   0,   1,   64,  0,   0,   0,   0,   128, 63,  17,
    0,   0,   8,   18,  32,  16,  0,   1,   0,   0,   0,   70,  14,  16,  0,
    0,   0,   0,   0,   70,  142, 32,  0,   0,   0,   0,   0,   2,   0,   0,
    0,   17,  0,   0,   8,   34,  32,  16,  0,   1,   0,   0,   0,   70,  14,
    16,  0,   0,   0,   0,   0,   70,  142, 32,  0,   0,   0,   0,   0,   3,
    0,   0,   0,   17,  0,   0,   8,   66,  32,  16,  0,   1,   0,   0,   0,
    70,  14,  16,  0,   0,   0,   0,   0,   70,  142, 32,  0,   0,   0,   0,
    0,   4,   0,   0,   0,   17,  0,   0,   8,   130, 32,  16,  0,   1,   0,
    0,   0,   70,  14,  16,  0,   0,   0,   0,   0,   70,  142, 32,  0,   0,
    0,   0,   0,   5,   0,   0,   0,   62,  0,   0,   1,   83,  84,  65,  84,
    116, 0,   0,   0,   9,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,
    0,   4,   0,   0,   0,   6,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0};

const BYTE g_main_ps[] =
   {68,  88,  66,  67,  38,  22,  44,  105, 183, 163, 92,  203, 37,  149, 28,
    47,  249, 242, 147, 206, 1,   0,   0,   0,   232, 3,   0,   0,   5,   0,
    0,   0,   52,  0,   0,   0,   128, 1,   0,   0,   216, 1,   0,   0,   12,
    2,   0,   0,   108, 3,   0,   0,   82,  68,  69,  70,  68,  1,   0,   0,
    1,   0,   0,   0,   72,  0,   0,   0,   1,   0,   0,   0,   28,  0,   0,
    0,   0,   4,   255, 255, 0,   1,   0,   0,   26,  1,   0,   0,   60,  0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,
    67,  111, 110, 115, 116, 97,  110, 116, 115, 0,   171, 171, 60,  0,   0,
    0,   4,   0,   0,   0,   96,  0,   0,   0,   112, 0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   192, 0,   0,   0,   0,   0,   0,   0,   12,
    0,   0,   0,   0,   0,   0,   0,   208, 0,   0,   0,   0,   0,   0,   0,
    224, 0,   0,   0,   16,  0,   0,   0,   12,  0,   0,   0,   0,   0,   0,
    0,   208, 0,   0,   0,   0,   0,   0,   0,   239, 0,   0,   0,   32,  0,
    0,   0,   64,  0,   0,   0,   0,   0,   0,   0,   4,   1,   0,   0,   0,
    0,   0,   0,   20,  1,   0,   0,   96,  0,   0,   0,   12,  0,   0,   0,
    2,   0,   0,   0,   208, 0,   0,   0,   0,   0,   0,   0,   112, 111, 115,
    105, 116, 105, 111, 110, 95,  109, 105, 110, 87,  83,  0,   171, 1,   0,
    3,   0,   1,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   112,
    111, 115, 105, 116, 105, 111, 110, 95,  109, 97,  120, 87,  83,  0,   112,
    114, 111, 106, 101, 99,  116, 105, 111, 110, 95,  109, 97,  116, 114, 105,
    120, 0,   171, 171, 171, 3,   0,   3,   0,   4,   0,   4,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   99,  111, 108, 111, 114, 0,   77,  105, 99,
    114, 111, 115, 111, 102, 116, 32,  40,  82,  41,  32,  72,  76,  83,  76,
    32,  83,  104, 97,  100, 101, 114, 32,  67,  111, 109, 112, 105, 108, 101,
    114, 32,  49,  48,  46,  49,  0,   171, 171, 73,  83,  71,  78,  80,  0,
    0,   0,   2,   0,   0,   0,   8,   0,   0,   0,   56,  0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,
    7,   7,   0,   0,   65,  0,   0,   0,   0,   0,   0,   0,   1,   0,   0,
    0,   3,   0,   0,   0,   1,   0,   0,   0,   15,  0,   0,   0,   78,  79,
    82,  77,  65,  76,  87,  83,  0,   83,  86,  95,  80,  111, 115, 105, 116,
    105, 111, 110, 0,   171, 171, 171, 79,  83,  71,  78,  44,  0,   0,   0,
    1,   0,   0,   0,   8,   0,   0,   0,   32,  0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   15,  0,
    0,   0,   83,  86,  95,  84,  97,  114, 103, 101, 116, 0,   171, 171, 83,
    72,  68,  82,  88,  1,   0,   0,   64,  0,   0,   0,   86,  0,   0,   0,
    89,  0,   0,   4,   70,  142, 32,  0,   0,   0,   0,   0,   7,   0,   0,
    0,   98,  16,  0,   3,   114, 16,  16,  0,   0,   0,   0,   0,   101, 0,
    0,   3,   242, 32,  16,  0,   0,   0,   0,   0,   104, 0,   0,   2,   1,
    0,   0,   0,   16,  0,   0,   7,   18,  0,   16,  0,   0,   0,   0,   0,
    70,  18,  16,  0,   0,   0,   0,   0,   70,  18,  16,  0,   0,   0,   0,
    0,   68,  0,   0,   5,   18,  0,   16,  0,   0,   0,   0,   0,   10,  0,
    16,  0,   0,   0,   0,   0,   56,  0,   0,   7,   114, 0,   16,  0,   0,
    0,   0,   0,   6,   0,   16,  0,   0,   0,   0,   0,   70,  18,  16,  0,
    0,   0,   0,   0,   16,  32,  0,   10,  18,  0,   16,  0,   0,   0,   0,
    0,   70,  2,   16,  0,   0,   0,   0,   0,   2,   64,  0,   0,   69,  97,
    235, 62,  139, 238, 93,  191, 11,  60,  69,  62,  0,   0,   0,   0,   75,
    0,   0,   5,   18,  0,   16,  0,   0,   0,   0,   0,   10,  0,   16,  0,
    0,   0,   0,   0,   0,   0,   0,   8,   34,  0,   16,  0,   0,   0,   0,
    0,   10,  0,   16,  128, 65,  0,   0,   0,   0,   0,   0,   0,   1,   64,
    0,   0,   0,   0,   128, 63,  56,  0,   0,   7,   34,  0,   16,  0,   0,
    0,   0,   0,   26,  0,   16,  0,   0,   0,   0,   0,   1,   64,  0,   0,
    0,   0,   128, 62,  50,  0,   0,   9,   18,  0,   16,  0,   0,   0,   0,
    0,   10,  0,   16,  0,   0,   0,   0,   0,   1,   64,  0,   0,   154, 153,
    25,  63,  26,  0,   16,  0,   0,   0,   0,   0,   56,  0,   0,   8,   114,
    32,  16,  0,   0,   0,   0,   0,   6,   0,   16,  0,   0,   0,   0,   0,
    70,  130, 32,  0,   0,   0,   0,   0,   6,   0,   0,   0,   54,  0,   0,
    5,   130, 32,  16,  0,   0,   0,   0,   0,   1,   64,  0,   0,   0,   0,
    128, 63,  62,  0,   0,   1,   83,  84,  65,  84,  116, 0,   0,   0,   11,
    0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,
    9,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0};

const std::array<float, 36 * 6> cube = {
   0.0f,  0.0f,  1.0f,  -1.0f, 0.0f,  0.0f,  0.0f,  1.0f,  1.0f,
   -1.0f, 0.0f,  0.0f,  0.0f,  1.0f,  0.0f,  -1.0f, 0.0f,  0.0f,

   0.0f,  0.0f,  1.0f,  -1.0f, 0.0f,  0.0f,  0.0f,  1.0f,  0.0f,
   -1.0f, 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  -1.0f, 0.0f,  0.0f,

   0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  -1.0f, 0.0f,  1.0f,  0.0f,
   0.0f,  0.0f,  -1.0f, 1.0f,  1.0f,  0.0f,  0.0f,  0.0f,  -1.0f,

   0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  -1.0f, 1.0f,  1.0f,  0.0f,
   0.0f,  0.0f,  -1.0f, 1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  -1.0f,

   1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
   1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f,

   1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
   1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,

   1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,  1.0f,
   0.0f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,

   1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
   0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,

   0.0f,  0.0f,  0.0f,  0.0f,  -1.0f, 0.0f,  1.0f,  0.0f,  0.0f,
   0.0f,  -1.0f, 0.0f,  1.0f,  0.0f,  1.0f,  0.0f,  -1.0f, 0.0f,

   0.0f,  0.0f,  0.0f,  0.0f,  -1.0f, 0.0f,  1.0f,  0.0f,  1.0f,
   0.0f,  -1.0f, 0.0f,  0.0f,  0.0f,  1.0f,  0.0f,  -1.0f, 0.0f,

   1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
   0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,

   1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
   0.0f,  1.0f,  0.0f,  1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,
};

}

Debug_world_aabb_draw::Debug_world_aabb_draw(ID3D11Device& device)
   : _device{copy_raw_com_ptr(device)}
{
}

void Debug_world_aabb_draw::draw(ID3D11DeviceContext2& dc, const World_inputs& world,
                                 const glm::mat4& projection_matrix,
                                 const Target_inputs& target) noexcept
{
   if (!_constants) initialize();

   dc.ClearState();

   const UINT vertex_stride = 24;
   const UINT vertex_offset = 0;

   dc.IASetInputLayout(_input_layout.get());
   dc.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
   dc.IASetVertexBuffers(0, 1, _vertex_buffer.get_ptr(), &vertex_stride, &vertex_offset);

   dc.VSSetShader(_vertex_shader.get(), nullptr, 0);

   dc.RSSetViewports(1, &target.viewport);
   dc.RSSetState(_rasterizer_state.get());

   dc.PSSetShader(_pixel_shader.get(), nullptr, 0);

   dc.OMSetRenderTargets(1, &target.rtv, target.dsv);
   dc.OMSetBlendState(nullptr, nullptr, 0xff'ff'ff'ffu);
   dc.OMSetDepthStencilState(_depth_stencil_state.get(), 0xffu);

   for (std::uint32_t i = 0; i < world.object_instances.size(); ++i) {
      const Object_instance& object = world.object_instances[i];
      const Game_model& game_model = world.game_models[object.game_model_index];
      const Model& model = world.models[game_model.lod0_index];

      const Bounding_box bbox =
         model.aabb *
         std::array{glm::vec4{object.rotation[0], object.positionWS.x},
                    glm::vec4{object.rotation[1], object.positionWS.y},
                    glm::vec4{object.rotation[2], object.positionWS.z}};

      const Constants constants = {.position_minWS = bbox.min,
                                   .position_maxWS = bbox.max,
                                   .projection_matrix = projection_matrix,
                                   .color = sample_color(i)};

      D3D11_MAPPED_SUBRESOURCE mapped = {};

      if (FAILED(dc.Map(_constants.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
         log_and_terminate("Failed to update constant buffer.");
      }

      std::memcpy(mapped.pData, &constants, sizeof(constants));

      dc.Unmap(_constants.get(), 0);

      ID3D11Buffer* constant_buffer = _constants.get();

      dc.VSSetConstantBuffers(0, 1, &constant_buffer);
      dc.PSSetConstantBuffers(0, 1, &constant_buffer);

      dc.Draw(cube.size() / 6, 0);
   }
}

void Debug_world_aabb_draw::draw(ID3D11DeviceContext2& dc, const World_inputs& world,
                                 const float camera_yaw, const float camera_pitch,
                                 const glm::vec3& camera_positioWS,
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

   dc.ClearDepthStencilView(target.dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
   dc.ClearRenderTargetView(target.rtv, std::array{0.0f, 0.0f, 0.0f, 1.0f}.data());

   draw(dc, world, projection_matrix, target);
}

void Debug_world_aabb_draw::initialize() noexcept
{
   const D3D11_BUFFER_DESC constant_buffer_desc = {
      .ByteWidth = sizeof(Constants),
      .Usage = D3D11_USAGE_DYNAMIC,
      .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
      .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
   };

   if (FAILED(_device->CreateBuffer(&constant_buffer_desc, nullptr,
                                    _constants.clear_and_assign()))) {
      log_and_terminate("Failed to create debug AABB draw resource.");
   }

   const D3D11_BUFFER_DESC vertex_buffer_desc = {
      .ByteWidth = sizeof(cube),
      .Usage = D3D11_USAGE_IMMUTABLE,
      .BindFlags = D3D11_BIND_VERTEX_BUFFER,
   };

   const D3D11_SUBRESOURCE_DATA vertex_buffer_init = {
      .pSysMem = cube.data(),
      .SysMemPitch = cube.size(),
      .SysMemSlicePitch = cube.size(),
   };

   if (FAILED(_device->CreateBuffer(&vertex_buffer_desc, &vertex_buffer_init,
                                    _vertex_buffer.clear_and_assign()))) {
      log_and_terminate("Failed to create debug AABB draw resource.");
   }

   const std::array<D3D11_INPUT_ELEMENT_DESC, 2> input_layout = {
      D3D11_INPUT_ELEMENT_DESC{
         .SemanticName = "POSITION",
         .Format = DXGI_FORMAT_R32G32B32_FLOAT,
         .AlignedByteOffset = 0,
      },
      D3D11_INPUT_ELEMENT_DESC{
         .SemanticName = "NORMAL",
         .Format = DXGI_FORMAT_R32G32B32_FLOAT,
         .AlignedByteOffset = 12,
      },
   };

   if (FAILED(_device->CreateInputLayout(input_layout.data(), input_layout.size(),
                                         &g_main_vs[0], sizeof(g_main_vs),
                                         _input_layout.clear_and_assign()))) {
      log_and_terminate("Failed to create debug AABB draw resource.");
   }

   if (FAILED(_device->CreateVertexShader(&g_main_vs[0], sizeof(g_main_vs), nullptr,
                                          _vertex_shader.clear_and_assign()))) {
      log_and_terminate("Failed to create debug AABB draw resource.");
   }

   const D3D11_RASTERIZER_DESC rasterizer_desc = {
      .FillMode = D3D11_FILL_SOLID,
      .CullMode = D3D11_CULL_BACK,
      .FrontCounterClockwise = true,
      .DepthClipEnable = true,
      .AntialiasedLineEnable = true,
   };

   if (FAILED(_device->CreateRasterizerState(&rasterizer_desc,
                                             _rasterizer_state.clear_and_assign()))) {
      log_and_terminate("Failed to create debug AABB draw resource.");
   }

   const D3D11_DEPTH_STENCIL_DESC depth_stencil_desc = {
      .DepthEnable = true,
      .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
      .DepthFunc = D3D11_COMPARISON_LESS_EQUAL,
   };

   if (FAILED(_device->CreateDepthStencilState(&depth_stencil_desc,
                                               _depth_stencil_state.clear_and_assign()))) {
      log_and_terminate("Failed to create debug AABB draw resource.");
   }

   if (FAILED(_device->CreatePixelShader(&g_main_ps[0], sizeof(g_main_ps), nullptr,
                                         _pixel_shader.clear_and_assign()))) {
      log_and_terminate("Failed to create debug AABB draw resource.");
   }
}

}