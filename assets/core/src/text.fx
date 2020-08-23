
#include "constants_list.hlsl"
#include "pixel_sampler_states.hlsl"

// clang-format off

#define ATLAS_GLYPH_COUNT 226

cbuffer FontAtlasIndex : register(MATERIAL_CB_INDEX)
{
   float2 bitmap_atlas_size;
   float4 bitmap_coords[ATLAS_GLYPH_COUNT];
   float4 msdf_coords[ATLAS_GLYPH_COUNT];
};

const static float4 interface_scale_offset = custom_constants[0];
const static float4 interface_color = ps_custom_constants[0];
const static float msdf_use_threshold = 1.1;

Texture2D<float> bitmap_glyph_atlas : register(t7);
Texture2D<float3> msdf_glyph_atlas : register(t8);

float4 transform_interface_position(float3 position)
{
   float4 positionPS =
      mul(float4(mul(float4(position, 1.0), world_matrix), 1.0), projection_matrix);

   positionPS.xy = positionPS.xy * interface_scale_offset.xy + interface_scale_offset.zw;
   positionPS.xy += vs_pixel_offset * positionPS.w;

   return positionPS;
}

struct Vs_input
{
   float3 position : POSITION;
   float2 texcoords : TEXCOORD;
};

struct Vs_output
{
   float2 bitmap_coords : BITMAP;
   float2 msdf_coords : MSDF;
   float4 positionPS : SV_Position;
   nointerpolation float2 viewport_size : VIEWPORT_SIZE;
};

Vs_output main_vs(Vs_input input)
{
   Vs_output output;

   output.positionPS = transform_interface_position(input.position);

   const int2 texcoords = (int2)input.texcoords;
   const int glyph = texcoords.x >> 2;
   const int2 coords_index = texcoords & 3;

   output.bitmap_coords = float2(bitmap_coords[glyph][coords_index.x], bitmap_coords[glyph][coords_index.y]);
   output.msdf_coords = float2(msdf_coords[glyph][coords_index.x], msdf_coords[glyph][coords_index.y]);
   output.viewport_size = float2(1.0, -1.0) / vs_pixel_offset;

   return output;
}

struct Gs_output {
   float2 bitmap_coords : BITMAP;
   float2 msdf_coords : MSDF;
   nointerpolation bool use_msdf : USE_MSDF;
   float4 positionPS : SV_Position;
};

[maxvertexcount(3)] 
void main_gs(triangle Vs_output input[3], inout TriangleStream<Gs_output> output) {
   float2 tri_positionSS[3] = {
      (input[0].positionPS.xy / input[0].positionPS.w * float2(0.5, -0.5) + 0.5) * input[0].viewport_size, 
      (input[1].positionPS.xy / input[1].positionPS.w * float2(0.5, -0.5) + 0.5) * input[0].viewport_size,
      (input[2].positionPS.xy / input[2].positionPS.w * float2(0.5, -0.5) + 0.5) * input[0].viewport_size};

   float2 lengthSS = float2(max(distance(tri_positionSS[0].x, tri_positionSS[1].x), 
                                distance(tri_positionSS[0].x, tri_positionSS[2].x)),
                            max(distance(tri_positionSS[0].y, tri_positionSS[1].y), 
                                distance(tri_positionSS[0].y, tri_positionSS[2].y)));
   float areaSS = lengthSS.x * lengthSS.y;
   
   float2 bitmap_coords[3] = {
      input[0].bitmap_coords * bitmap_atlas_size, 
      input[1].bitmap_coords * bitmap_atlas_size, 
      input[2].bitmap_coords * bitmap_atlas_size};

   float2 length_bitmap_coords = float2(max(distance(bitmap_coords[0].x, bitmap_coords[1].x), 
                                            distance(bitmap_coords[0].x, bitmap_coords[2].x)),
                                        max(distance(bitmap_coords[0].y, bitmap_coords[1].y), 
                                            distance(bitmap_coords[0].y, bitmap_coords[2].y)));
   float area_bitmap_coords = length_bitmap_coords.x * length_bitmap_coords.y;

   bool use_msdf = (areaSS / area_bitmap_coords) >= msdf_use_threshold;

   for (uint i = 0; i < 3; i++) {
      Gs_output vertex;
      vertex.bitmap_coords = input[i].bitmap_coords;
      vertex.msdf_coords = input[i].msdf_coords;
      vertex.positionPS = input[i].positionPS;
      vertex.use_msdf = use_msdf;
      output.Append(vertex);
   }
}

float median(float3 v)
{
   return max(min(v.r, v.g), min(max(v.r, v.g), v.b));
}

float4 main_ps(float2 bitmap_coords : BITMAP, float2 msdf_coords : MSDF,
               nointerpolation bool use_msdf : USE_MSDF) : SV_Target0
{
   float alpha;

   [branch]
   if (!use_msdf) {
      alpha = bitmap_glyph_atlas.SampleLevel(linear_clamp_sampler, bitmap_coords, 0);
   }
   else {

      const float3 distances = msdf_glyph_atlas.SampleLevel(linear_clamp_sampler, msdf_coords, 0);
      const float distance = median(distances);

      alpha = distance >= 0.5f;

      [branch]
      if (fwidth(alpha) != 0.0) {
         alpha = 0.0;
         
         static const int2 sample_offsets[8] = {
            int2(1, -3),
            int2(-1, 3),
            int2(5, 1),
            int2(-3, -5),
            int2(-5, 5),
            int2(-7, -1),
            int2(3,  7),
            int2(7, -7)
         };

         [unroll]
         for (uint i = 0; i < 8; ++i) {
            const float2 sample_coords = EvaluateAttributeSnapped(msdf_coords, sample_offsets[i]);

            const float3 sample_distances = msdf_glyph_atlas.SampleLevel(linear_clamp_sampler, sample_coords, 0);
            const float sample_distance = median(sample_distances);
         
            alpha += (sample_distance >= 0.5f);
         }

         alpha /= 8.0;
      }
   }
   
   return float4(interface_color.rgb, interface_color.a * alpha);
}
