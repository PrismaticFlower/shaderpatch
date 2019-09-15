cbuffer SceneBlurConstants : register(b0)
{
   float2 blur_dir_size;
   float factor;
}

Texture2D<float3> scene_texture;

SamplerState samp;

float4 blur_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   const static float weights[9] = {0.13994993, 0.24148224, 0.13345071,
                                    0.04506128, 0.00897960, 0.00099466,
                                    0.00005526, 0.00000127, 0.00000001};
   const static float offsets[9] = {0.00000000,  1.45454545,  3.39393939,
                                    5.33333333,  7.27272727,  9.21212121,
                                    11.15151515, 13.09090909, 15.03030303};

   float3 color = scene_texture.SampleLevel(samp, texcoords, 0) * weights[0];

   [unroll] for (int i = 1; i < 9; ++i)
   {
      color +=
         scene_texture.SampleLevel(samp, texcoords + (offsets[i] * blur_dir_size), 0) *
         weights[i];
      color +=
         scene_texture.SampleLevel(samp, texcoords - (offsets[i] * blur_dir_size), 0) *
         weights[i];
   }

   return float4(color, 1.0);
}

float4 overlay_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   return float4(scene_texture.SampleLevel(samp, texcoords, 0), factor);
}
