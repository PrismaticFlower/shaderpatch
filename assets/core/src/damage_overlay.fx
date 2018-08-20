
#include "fullscreen_tri_vs.hlsl"
#include "pixel_utilities.hlsl"

sampler2D scene_sampler : register(s0);

float4 damage_color : register(c60);
float2 blur_step_sample_count : register(c61);

const static float blur_step = blur_step_sample_count.x;
const static float sample_count = blur_step_sample_count.y;

float2 polar(float2 uv)
{
   float x = sqrt(uv.x * uv.x + uv.y * uv.y);
   float y = atan2(uv.y, uv.x);

   return float2(x, y);
}

float2 cartesian(float2 uv)
{
   float x = uv.x * cos(uv.y);
   float y = uv.x * sin(uv.y);

   return float2(x, y);
}

float4 apply_ps(float2 texcoords : TEXCOORD) : COLOR
{
   float fade = smoothstep(0.05, 0.55, distance(texcoords, float2(0.5, 0.5)));

   [branch] if (fade < 0.000001) discard;

   float3 color = tex2Dlod(scene_sampler, texcoords, 0).rgb;

   [loop] for (float i = 1.0; i < sample_count; i += 1.0) {
      float2 uv = polar(texcoords * 2.0 - 1.0);
      uv.x += blur_step * i;
      uv = cartesian(uv) * 0.5 + 0.5;

      color.rgb += tex2Dlod(scene_sampler, uv, 0).rgb;
   }

   color.rgb /= sample_count;

   color = lerp(color, damage_color.rgb, damage_color.a);

   return float4(color, fade);
}