#include "debug_world_textured_draw.hpp"

#include "../../imgui/imgui.h"
#include "../../logger.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace sp::shadows {

namespace {

struct alignas(16) Constants {
   glm::vec3 position_decompress_mul;
   std::uint32_t pad0;

   glm::vec3 position_decompress_add;
   std::uint32_t pad1;

   std::array<glm::vec4, 3> rotation_matrix;
   glm::vec3 translateWS;
   std::uint32_t pad2;

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
   
   float3x3 rotation_matrix;
   float3 translateWS;

   float4x4 projection_matrix;
};

struct Vertex_output {
   float2 texcoords : TEXCOORD;
   float3 position_offsetWS : POSITIONWS;
   float4 positionPS : SV_Position;
};

Texture2D<float4> color_map;
SamplerState texture_sampler;

Vertex_output main_vs(int position_XCS : POSITION_X, int position_YCS : POSITION_Y, int position_ZCS : POSITION_Z,
                      int texcoord_X : TEXCOORD_X, int texcoord_Y : TEXCOORD_Y)
{
   int3 positionCS = {position_XCS, position_YCS, position_ZCS};
   float3 positionOS = positionCS.xyz * position_decompress_mul + position_decompress_add;
   float3 position_almostWS = mul(positionOS, rotation_matrix);
   float3 positionWS = position_almostWS + translateWS;
   float2 texcoords = float2(texcoord_X, texcoord_Y) * (1.0 / 2048.0);

   Vertex_output output;

   output.texcoords = texcoords;
   output.position_offsetWS = position_almostWS;
   output.positionPS = mul(float4(positionWS, 1.0), projection_matrix);

   return output;
}

float4 main_ps(Vertex_output input) : SV_Target0
{   
   const float4 color = color_map.Sample(texture_sampler, input.texcoords);

   if (color.a < 0.5) discard;

   const float3 light_normalWS = normalize(float3(159.264923, -300.331013, 66.727310));
   float3 normalWS = normalize(cross(ddx(input.position_offsetWS), ddy(input.position_offsetWS)));

   float light = sqrt(saturate(dot(normalize(normalWS), light_normalWS)));

   light = light + (1.0 - light) * 0.75;

   return float4(light * color.rgb, 1.0);
}
#endif

const BYTE g_main_vs[] =
   {68,  88,  66,  67,  103, 41,  228, 28,  73,  89,  27,  6,   68,  124, 245,
    210, 120, 151, 45,  155, 1,   0,   0,   0,   240, 5,   0,   0,   5,   0,
    0,   0,   52,  0,   0,   0,   204, 1,   0,   0,   140, 2,   0,   0,   4,
    3,   0,   0,   116, 5,   0,   0,   82,  68,  69,  70,  144, 1,   0,   0,
    1,   0,   0,   0,   72,  0,   0,   0,   1,   0,   0,   0,   28,  0,   0,
    0,   0,   4,   254, 255, 0,   1,   0,   0,   104, 1,   0,   0,   60,  0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,
    67,  111, 110, 115, 116, 97,  110, 116, 115, 0,   171, 171, 60,  0,   0,
    0,   5,   0,   0,   0,   96,  0,   0,   0,   160, 0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   216, 0,   0,   0,   0,   0,   0,   0,   12,
    0,   0,   0,   2,   0,   0,   0,   240, 0,   0,   0,   0,   0,   0,   0,
    0,   1,   0,   0,   16,  0,   0,   0,   12,  0,   0,   0,   2,   0,   0,
    0,   240, 0,   0,   0,   0,   0,   0,   0,   24,  1,   0,   0,   32,  0,
    0,   0,   44,  0,   0,   0,   2,   0,   0,   0,   40,  1,   0,   0,   0,
    0,   0,   0,   56,  1,   0,   0,   80,  0,   0,   0,   12,  0,   0,   0,
    2,   0,   0,   0,   240, 0,   0,   0,   0,   0,   0,   0,   68,  1,   0,
    0,   96,  0,   0,   0,   64,  0,   0,   0,   2,   0,   0,   0,   88,  1,
    0,   0,   0,   0,   0,   0,   112, 111, 115, 105, 116, 105, 111, 110, 95,
    100, 101, 99,  111, 109, 112, 114, 101, 115, 115, 95,  109, 117, 108, 0,
    1,   0,   3,   0,   1,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   112, 111, 115, 105, 116, 105, 111, 110, 95,  100, 101, 99,  111, 109,
    112, 114, 101, 115, 115, 95,  97,  100, 100, 0,   114, 111, 116, 97,  116,
    105, 111, 110, 95,  109, 97,  116, 114, 105, 120, 0,   3,   0,   3,   0,
    3,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   116, 114, 97,
    110, 115, 108, 97,  116, 101, 87,  83,  0,   112, 114, 111, 106, 101, 99,
    116, 105, 111, 110, 95,  109, 97,  116, 114, 105, 120, 0,   171, 171, 3,
    0,   3,   0,   4,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    77,  105, 99,  114, 111, 115, 111, 102, 116, 32,  40,  82,  41,  32,  72,
    76,  83,  76,  32,  83,  104, 97,  100, 101, 114, 32,  67,  111, 109, 112,
    105, 108, 101, 114, 32,  49,  48,  46,  49,  0,   73,  83,  71,  78,  184,
    0,   0,   0,   5,   0,   0,   0,   8,   0,   0,   0,   128, 0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,   0,   0,   0,
    0,   1,   1,   0,   0,   139, 0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   2,   0,   0,   0,   1,   0,   0,   0,   1,   1,   0,   0,   150,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,
    2,   0,   0,   0,   1,   1,   0,   0,   161, 0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   2,   0,   0,   0,   3,   0,   0,   0,   1,   1,
    0,   0,   172, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,
    0,   0,   0,   4,   0,   0,   0,   1,   1,   0,   0,   80,  79,  83,  73,
    84,  73,  79,  78,  95,  88,  0,   80,  79,  83,  73,  84,  73,  79,  78,
    95,  89,  0,   80,  79,  83,  73,  84,  73,  79,  78,  95,  90,  0,   84,
    69,  88,  67,  79,  79,  82,  68,  95,  88,  0,   84,  69,  88,  67,  79,
    79,  82,  68,  95,  89,  0,   171, 79,  83,  71,  78,  112, 0,   0,   0,
    3,   0,   0,   0,   8,   0,   0,   0,   80,  0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   3,   12,
    0,   0,   89,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   3,
    0,   0,   0,   1,   0,   0,   0,   7,   8,   0,   0,   100, 0,   0,   0,
    0,   0,   0,   0,   1,   0,   0,   0,   3,   0,   0,   0,   2,   0,   0,
    0,   15,  0,   0,   0,   84,  69,  88,  67,  79,  79,  82,  68,  0,   80,
    79,  83,  73,  84,  73,  79,  78,  87,  83,  0,   83,  86,  95,  80,  111,
    115, 105, 116, 105, 111, 110, 0,   83,  72,  68,  82,  104, 2,   0,   0,
    64,  0,   1,   0,   154, 0,   0,   0,   89,  0,   0,   4,   70,  142, 32,
    0,   0,   0,   0,   0,   10,  0,   0,   0,   95,  0,   0,   3,   18,  16,
    16,  0,   0,   0,   0,   0,   95,  0,   0,   3,   18,  16,  16,  0,   1,
    0,   0,   0,   95,  0,   0,   3,   18,  16,  16,  0,   2,   0,   0,   0,
    95,  0,   0,   3,   18,  16,  16,  0,   3,   0,   0,   0,   95,  0,   0,
    3,   18,  16,  16,  0,   4,   0,   0,   0,   101, 0,   0,   3,   50,  32,
    16,  0,   0,   0,   0,   0,   101, 0,   0,   3,   114, 32,  16,  0,   1,
    0,   0,   0,   103, 0,   0,   4,   242, 32,  16,  0,   2,   0,   0,   0,
    1,   0,   0,   0,   104, 0,   0,   2,   2,   0,   0,   0,   43,  0,   0,
    5,   18,  0,   16,  0,   0,   0,   0,   0,   10,  16,  16,  0,   3,   0,
    0,   0,   43,  0,   0,   5,   34,  0,   16,  0,   0,   0,   0,   0,   10,
    16,  16,  0,   4,   0,   0,   0,   56,  0,   0,   10,  50,  32,  16,  0,
    0,   0,   0,   0,   70,  0,   16,  0,   0,   0,   0,   0,   2,   64,  0,
    0,   0,   0,   0,   58,  0,   0,   0,   58,  0,   0,   0,   0,   0,   0,
    0,   0,   43,  0,   0,   5,   18,  0,   16,  0,   0,   0,   0,   0,   10,
    16,  16,  0,   0,   0,   0,   0,   43,  0,   0,   5,   34,  0,   16,  0,
    0,   0,   0,   0,   10,  16,  16,  0,   1,   0,   0,   0,   43,  0,   0,
    5,   66,  0,   16,  0,   0,   0,   0,   0,   10,  16,  16,  0,   2,   0,
    0,   0,   50,  0,   0,   11,  114, 0,   16,  0,   0,   0,   0,   0,   70,
    2,   16,  0,   0,   0,   0,   0,   70,  130, 32,  0,   0,   0,   0,   0,
    0,   0,   0,   0,   70,  130, 32,  0,   0,   0,   0,   0,   1,   0,   0,
    0,   16,  0,   0,   8,   18,  0,   16,  0,   1,   0,   0,   0,   70,  2,
    16,  0,   0,   0,   0,   0,   70,  130, 32,  0,   0,   0,   0,   0,   2,
    0,   0,   0,   16,  0,   0,   8,   34,  0,   16,  0,   1,   0,   0,   0,
    70,  2,   16,  0,   0,   0,   0,   0,   70,  130, 32,  0,   0,   0,   0,
    0,   3,   0,   0,   0,   16,  0,   0,   8,   66,  0,   16,  0,   1,   0,
    0,   0,   70,  2,   16,  0,   0,   0,   0,   0,   70,  130, 32,  0,   0,
    0,   0,   0,   4,   0,   0,   0,   54,  0,   0,   5,   114, 32,  16,  0,
    1,   0,   0,   0,   70,  2,   16,  0,   1,   0,   0,   0,   0,   0,   0,
    8,   114, 0,   16,  0,   0,   0,   0,   0,   70,  2,   16,  0,   1,   0,
    0,   0,   70,  130, 32,  0,   0,   0,   0,   0,   5,   0,   0,   0,   54,
    0,   0,   5,   130, 0,   16,  0,   0,   0,   0,   0,   1,   64,  0,   0,
    0,   0,   128, 63,  17,  0,   0,   8,   18,  32,  16,  0,   2,   0,   0,
    0,   70,  14,  16,  0,   0,   0,   0,   0,   70,  142, 32,  0,   0,   0,
    0,   0,   6,   0,   0,   0,   17,  0,   0,   8,   34,  32,  16,  0,   2,
    0,   0,   0,   70,  14,  16,  0,   0,   0,   0,   0,   70,  142, 32,  0,
    0,   0,   0,   0,   7,   0,   0,   0,   17,  0,   0,   8,   66,  32,  16,
    0,   2,   0,   0,   0,   70,  14,  16,  0,   0,   0,   0,   0,   70,  142,
    32,  0,   0,   0,   0,   0,   8,   0,   0,   0,   17,  0,   0,   8,   130,
    32,  16,  0,   2,   0,   0,   0,   70,  14,  16,  0,   0,   0,   0,   0,
    70,  142, 32,  0,   0,   0,   0,   0,   9,   0,   0,   0,   62,  0,   0,
    1,   83,  84,  65,  84,  116, 0,   0,   0,   18,  0,   0,   0,   2,   0,
    0,   0,   0,   0,   0,   0,   8,   0,   0,   0,   10,  0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,   0,
    0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0};

const BYTE g_main_ps[] =
   {68,  88,  66,  67,  243, 226, 95,  182, 226, 188, 62,  34,  140, 128, 255,
    58,  69,  77,  33,  0,   1,   0,   0,   0,   20,  4,   0,   0,   5,   0,
    0,   0,   52,  0,   0,   0,   220, 0,   0,   0,   84,  1,   0,   0,   136,
    1,   0,   0,   152, 3,   0,   0,   82,  68,  69,  70,  160, 0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,   28,  0,   0,
    0,   0,   4,   255, 255, 0,   1,   0,   0,   118, 0,   0,   0,   92,  0,
    0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,
    108, 0,   0,   0,   2,   0,   0,   0,   5,   0,   0,   0,   4,   0,   0,
    0,   255, 255, 255, 255, 0,   0,   0,   0,   1,   0,   0,   0,   12,  0,
    0,   0,   116, 101, 120, 116, 117, 114, 101, 95,  115, 97,  109, 112, 108,
    101, 114, 0,   99,  111, 108, 111, 114, 95,  109, 97,  112, 0,   77,  105,
    99,  114, 111, 115, 111, 102, 116, 32,  40,  82,  41,  32,  72,  76,  83,
    76,  32,  83,  104, 97,  100, 101, 114, 32,  67,  111, 109, 112, 105, 108,
    101, 114, 32,  49,  48,  46,  49,  0,   171, 171, 73,  83,  71,  78,  112,
    0,   0,   0,   3,   0,   0,   0,   8,   0,   0,   0,   80,  0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,
    0,   3,   3,   0,   0,   89,  0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   3,   0,   0,   0,   1,   0,   0,   0,   7,   7,   0,   0,   100,
    0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   3,   0,   0,   0,
    2,   0,   0,   0,   15,  0,   0,   0,   84,  69,  88,  67,  79,  79,  82,
    68,  0,   80,  79,  83,  73,  84,  73,  79,  78,  87,  83,  0,   83,  86,
    95,  80,  111, 115, 105, 116, 105, 111, 110, 0,   79,  83,  71,  78,  44,
    0,   0,   0,   1,   0,   0,   0,   8,   0,   0,   0,   32,  0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,
    0,   15,  0,   0,   0,   83,  86,  95,  84,  97,  114, 103, 101, 116, 0,
    171, 171, 83,  72,  68,  82,  8,   2,   0,   0,   64,  0,   0,   0,   130,
    0,   0,   0,   90,  0,   0,   3,   0,   96,  16,  0,   0,   0,   0,   0,
    88,  24,  0,   4,   0,   112, 16,  0,   0,   0,   0,   0,   85,  85,  0,
    0,   98,  16,  0,   3,   50,  16,  16,  0,   0,   0,   0,   0,   98,  16,
    0,   3,   114, 16,  16,  0,   1,   0,   0,   0,   101, 0,   0,   3,   242,
    32,  16,  0,   0,   0,   0,   0,   104, 0,   0,   2,   4,   0,   0,   0,
    69,  0,   0,   9,   242, 0,   16,  0,   0,   0,   0,   0,   70,  16,  16,
    0,   0,   0,   0,   0,   70,  126, 16,  0,   0,   0,   0,   0,   0,   96,
    16,  0,   0,   0,   0,   0,   49,  0,   0,   7,   130, 0,   16,  0,   0,
    0,   0,   0,   58,  0,   16,  0,   0,   0,   0,   0,   1,   64,  0,   0,
    0,   0,   0,   63,  13,  0,   4,   3,   58,  0,   16,  0,   0,   0,   0,
    0,   11,  0,   0,   5,   114, 0,   16,  0,   1,   0,   0,   0,   38,  25,
    16,  0,   1,   0,   0,   0,   12,  0,   0,   5,   114, 0,   16,  0,   2,
    0,   0,   0,   150, 20,  16,  0,   1,   0,   0,   0,   56,  0,   0,   7,
    114, 0,   16,  0,   3,   0,   0,   0,   70,  2,   16,  0,   1,   0,   0,
    0,   70,  2,   16,  0,   2,   0,   0,   0,   50,  0,   0,   10,  114, 0,
    16,  0,   1,   0,   0,   0,   38,  9,   16,  0,   1,   0,   0,   0,   150,
    4,   16,  0,   2,   0,   0,   0,   70,  2,   16,  128, 65,  0,   0,   0,
    3,   0,   0,   0,   16,  0,   0,   7,   130, 0,   16,  0,   0,   0,   0,
    0,   70,  2,   16,  0,   1,   0,   0,   0,   70,  2,   16,  0,   1,   0,
    0,   0,   68,  0,   0,   5,   130, 0,   16,  0,   0,   0,   0,   0,   58,
    0,   16,  0,   0,   0,   0,   0,   56,  0,   0,   7,   114, 0,   16,  0,
    1,   0,   0,   0,   246, 15,  16,  0,   0,   0,   0,   0,   70,  2,   16,
    0,   1,   0,   0,   0,   16,  32,  0,   10,  130, 0,   16,  0,   0,   0,
    0,   0,   70,  2,   16,  0,   1,   0,   0,   0,   2,   64,  0,   0,   69,
    97,  235, 62,  139, 238, 93,  191, 11,  60,  69,  62,  0,   0,   0,   0,
    75,  0,   0,   5,   130, 0,   16,  0,   0,   0,   0,   0,   58,  0,   16,
    0,   0,   0,   0,   0,   0,   0,   0,   8,   18,  0,   16,  0,   1,   0,
    0,   0,   58,  0,   16,  128, 65,  0,   0,   0,   0,   0,   0,   0,   1,
    64,  0,   0,   0,   0,   128, 63,  50,  0,   0,   9,   130, 0,   16,  0,
    0,   0,   0,   0,   10,  0,   16,  0,   1,   0,   0,   0,   1,   64,  0,
    0,   0,   0,   64,  63,  58,  0,   16,  0,   0,   0,   0,   0,   56,  0,
    0,   7,   114, 32,  16,  0,   0,   0,   0,   0,   70,  2,   16,  0,   0,
    0,   0,   0,   246, 15,  16,  0,   0,   0,   0,   0,   54,  0,   0,   5,
    130, 32,  16,  0,   0,   0,   0,   0,   1,   64,  0,   0,   0,   0,   128,
    63,  62,  0,   0,   1,   83,  84,  65,  84,  116, 0,   0,   0,   17,  0,
    0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   13,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0};

}

Debug_world_textured_draw::Debug_world_textured_draw(ID3D11Device& device)
   : _device{copy_raw_com_ptr(device)}
{
}

void Debug_world_textured_draw::draw(ID3D11DeviceContext2& dc, const World_inputs& world,
                                     const glm::mat4& projection_matrix,
                                     ID3D11Buffer* index_buffer,
                                     ID3D11Buffer* vertex_buffer,
                                     const Target_inputs& target) noexcept
{
   if (!_input_layout) initialize();

   dc.ClearState();

   dc.ClearDepthStencilView(target.dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
   if (target.rtv)
      dc.ClearRenderTargetView(target.rtv, std::array{0.0f, 0.0f, 0.0f, 1.0f}.data());

   dc.IASetInputLayout(_input_layout.get());
   dc.IASetIndexBuffer(index_buffer, DXGI_FORMAT_R16_UINT, 0);

   dc.VSSetShader(_vertex_shader.get(), nullptr, 0);

   dc.RSSetViewports(1, &target.viewport);

   dc.PSSetShader(_pixel_shader.get(), nullptr, 0);

   dc.OMSetRenderTargets(1, &target.rtv, target.dsv);
   dc.OMSetBlendState(nullptr, nullptr, 0xff'ff'ff'ffu);
   dc.OMSetDepthStencilState(_depth_stencil_state.get(), 0xffu);

   for (std::uint32_t i = 0; i < world.object_instances.size(); ++i) {
      const Object_instance& object = world.object_instances[i];
      const Game_model& game_model = world.game_models[object.game_model_index];
      const Model& model = world.models[game_model.lod0_index];

      const Constants constants = {
         .position_decompress_mul = model.position_decompress_mul,
         .position_decompress_add = model.position_decompress_add,
         .rotation_matrix = {glm::vec4{object.rotation[0], 0.0f},
                             glm::vec4{object.rotation[1], 0.0f},
                             glm::vec4{object.rotation[2], 0.0f}},
         .translateWS = object.positionWS,
         .projection_matrix = projection_matrix,
      };

      D3D11_MAPPED_SUBRESOURCE mapped = {};

      if (FAILED(dc.Map(_constants.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
         log_and_terminate("Failed to update constant buffer.");
      }

      std::memcpy(mapped.pData, &constants, sizeof(constants));

      dc.Unmap(_constants.get(), 0);

      ID3D11Buffer* constant_buffer = _constants.get();

      dc.VSSetConstantBuffers(0, 1, &constant_buffer);
      dc.PSSetConstantBuffers(0, 1, &constant_buffer);
      dc.PSSetSamplers(0, 1, _sampler.get_ptr());

      const UINT offset = 0;
      const UINT textured_stride = 10;

      dc.IASetVertexBuffers(0, 1, &vertex_buffer, &textured_stride, &offset);
      dc.RSSetState(_rasterizer_state.get());

      for (const Model_segment_hardedged& segment : model.hardedged_segments) {
         dc.IASetPrimitiveTopology(segment.topology);
         dc.PSSetShaderResources(0, 1, world.texture_table.get_ptr(segment.texture_index));
         dc.DrawIndexed(segment.index_count, segment.start_index, segment.base_vertex);
      }

      dc.RSSetState(_doublesided_rasterizer_state.get());

      for (const Model_segment_hardedged& segment : model.hardedged_doublesided_segments) {
         dc.IASetPrimitiveTopology(segment.topology);
         dc.PSSetShaderResources(0, 1, world.texture_table.get_ptr(segment.texture_index));
         dc.DrawIndexed(segment.index_count, segment.start_index, segment.base_vertex);
      }
   }
}

void Debug_world_textured_draw::draw(ID3D11DeviceContext2& dc, const World_inputs& world,
                                     const float camera_yaw, const float camera_pitch,
                                     const glm::vec3& camera_positioWS,
                                     ID3D11Buffer* index_buffer,
                                     ID3D11Buffer* vertex_buffer,
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

void Debug_world_textured_draw::initialize() noexcept
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

   const std::array<D3D11_INPUT_ELEMENT_DESC, 5> input_layout = {
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
      D3D11_INPUT_ELEMENT_DESC{
         .SemanticName = "TEXCOORD_X",
         .Format = DXGI_FORMAT_R16_SINT,
         .AlignedByteOffset = 6,
      },
      D3D11_INPUT_ELEMENT_DESC{
         .SemanticName = "TEXCOORD_Y",
         .Format = DXGI_FORMAT_R16_SINT,
         .AlignedByteOffset = 8,
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

   const D3D11_SAMPLER_DESC sampler_desc = {
      .Filter = D3D11_FILTER_ANISOTROPIC,
      .AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
      .AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
      .AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
      .MipLODBias = 0.0f,
      .MaxAnisotropy = 16,
      .ComparisonFunc = D3D11_COMPARISON_ALWAYS,
      .MinLOD = 0.0f,
      .MaxLOD = FLT_MAX,
   };

   if (FAILED(_device->CreateSamplerState(&sampler_desc, _sampler.clear_and_assign()))) {
      log_and_terminate("Failed to create debug model draw resource.");
   }
}

}