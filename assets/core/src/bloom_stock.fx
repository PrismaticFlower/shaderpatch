
#include "vertex_utilities.hlsl"

struct Vs_input
{
   float4 position : POSITION;
   float4 texcoord : TEXCOORD;
};

struct Vs_screenspace_output
{
   float4 position : POSITION;
   float2 texcoord : TEXCOORD;
};

struct Vs_bloomfilter_output
{
   float4 position : POSITION;
   float2 texcoord_0 : TEXCOORD0;
   float2 texcoord_1 : TEXCOORD1;
   float2 texcoord_2 : TEXCOORD2;
   float2 texcoord_3 : TEXCOORD3;
};

sampler buffer_sampler;

float4 hdr_constant : register(vs, c[CUSTOM_CONST_MIN + 1]);
float4 hdr_tex_scale : register(vs, c[CUSTOM_CONST_MIN + 2]);

Vs_screenspace_output screenspace_vs(Vs_input input)
{
   Vs_screenspace_output output;

   output.texcoord = decompress_texcoords(input.texcoord);

   float4 position = decompress_position(input.position);

   position.xy = position.xy + -hdr_constant.zw;
   position.xy *= 2.0;
   position.xy = position.xy * float2(1.0, -1.0) + float2(-1.0, 1.0);
   position.zw = float2(0.5, 1.0);

   output.position = position;

   return output;
}

Vs_bloomfilter_output bloomfilter_vs(Vs_input input)
{
   Vs_bloomfilter_output output;

   float2 texcoords = decompress_texcoords(input.texcoord);

   output.texcoord_0 = texcoords * hdr_tex_scale.xy + hdr_tex_scale.zw;

   output.texcoord_1.x = texcoords.x * hdr_tex_scale.x + -hdr_tex_scale.z;
   output.texcoord_1.y = texcoords.y * hdr_tex_scale.y + hdr_tex_scale.w;

   output.texcoord_2.x = texcoords.x * hdr_tex_scale.x + hdr_tex_scale.z;
   output.texcoord_2.y = texcoords.y * hdr_tex_scale.y + -hdr_tex_scale.w;

   output.texcoord_3 = texcoords * hdr_tex_scale.xy + -hdr_tex_scale.zw;

   float4 position = decompress_position(input.position);

   position.xy = position.xy + -hdr_constant.zw;
   position.xy *= 2.0;
   position.xy = position.xy * float2(1.0, -1.0) + float2(-1.0, 1.0);
   position.zw = float2(0.5, 1.0);

   output.position = position;

   return output;
}

float4 ps_constants[3] : register(ps, c[0]);

float4 glowthreshold_ps(float2 texcoord : TEXCOORD) : COLOR
{
   float4 buffer_color = tex2D(buffer_sampler, texcoord);

   float4 color;

   color.rgb = buffer_color.rgb - ps_constants[0].aaa;
   color.rgb = clamp(color.rgb, float3(0, 0, 0), float3(1, 1, 1));

   color.rgba = dot(color.rgb, ps_constants[0].rgb);

   color.a += 0.45;

   if (color.a > 0.5) color.rgb = buffer_color.rgb;
   else color.rgb = float3(0.0, 0.0, 0.0);

   return color;
}

float4 luminance_ps(float2 texcoord : TEXCOORD) : COLOR
{
   float4 buffer_color = tex2D(buffer_sampler, texcoord);

   return dot(buffer_color.rgb, ps_constants[0].rgb);
}

float4 screenspace_ps(float2 texcoord : TEXCOORD) : COLOR
{
   float4 buffer_color = tex2D(buffer_sampler, texcoord);

   return float4(buffer_color.rgb * ps_constants[2].rgb, ps_constants[2].a);
}

struct Ps_bloomfilter_input
{
   float2 texcoord_0 : TEXCOORD0;
   float2 texcoord_1 : TEXCOORD1;
   float2 texcoord_2 : TEXCOORD2;
   float2 texcoord_3 : TEXCOORD3;
};

float4 bloomfilter_ps(Ps_bloomfilter_input input) : COLOR
{
   float4 buffer_color_0 = tex2D(buffer_sampler, input.texcoord_0);
   float4 buffer_color_1 = tex2D(buffer_sampler, input.texcoord_1);
   float4 buffer_color_2 = tex2D(buffer_sampler, input.texcoord_2);
   float4 buffer_color_3 = tex2D(buffer_sampler, input.texcoord_3);

   float4 samples[2];

   samples[0] = buffer_color_0 * ps_constants[1].aaaa;
   samples[0] += (buffer_color_1 * ps_constants[1].aaaa);

   samples[1] = buffer_color_2 * ps_constants[1].aaaa;
   samples[1] += (buffer_color_3 * ps_constants[1].aaaa);

   return samples[0] + samples[1];
}