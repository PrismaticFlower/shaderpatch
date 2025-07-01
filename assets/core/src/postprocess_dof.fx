
// Useful references used while making this effect.

// UE4 Optimized Post-Effects, Adrian Courrèges - https://www.adriancourreges.com/blog/2018/12/02/ue4-optimized-post-effects/
// MJP's DoF implementation inThe Baking Lab - https://github.com/TheRealMJP/BakingLab
// The Skylanders SWAP Force Depth-of-Field Shader, Morgan McGuire - https://casual-effects.blogspot.com/2013/09/the-skylanders-swap-force-depth-of.html
// CryENGINE 3 Graphics Gems, Tiago Sousa - https://web.archive.org/web/20170611154038/http://www.crytek.com/download/Sousa_Graphics_Gems_CryENGINE3.pdf
// Efficiently Simulating the Bokeh of Polygonal Apertures in a Post-Process Depth of Field Shader, L. McIntosh, B. E. Riecke and S. DiPaola - http://ivizlab.sfu.ca/papers/cgf2012.pdf
// Five Rendering Ideas from Battlefield 3 and Need For Speed: The Run, John White, Colin Barré-Brisebois - https://www.gamedevs.org/uploads/rendering-in-battlefield3.pdf

Texture2D<float3> scene_input : register(t0);
Texture2D<float> depth_input : register(t1);

Texture2D near_input : register(t0);
Texture2D<float> near_mask_input : register(t1);
Texture2D far_input : register(t2);

Texture2D floodfill_near_input : register(t0);
Texture2D floodfill_far_input : register(t1);

Texture2D compose_near_input : register(t0);
Texture2D compose_far_input : register(t1);
Texture2D<float3> compose_input : register(t2);
Texture2D<float> compose_depth_input : register(t3);

Texture2D tile_near_coc_input : register(t0);
RWTexture2D<float> tile_near_coc_output : register(u0);

Texture2D<float> blur_near_mask_input  : register(t0);

SamplerState linear_sample : register(s0);
SamplerState linear_clamp_sample : register(s1);

cbuffer PostprocessDoFConstants : register(b1)
{
   float proj_from_view_m33;
   float proj_from_view_m43;
   float focus_distance;
   float coc_mul;
   float2 target_size;
   float2 inv_target_size;
   float2 near_mask_size;
   float2 inv_near_mask_size;
}

#ifdef GATHER_QUARTER_RESOLUTION
static const float max_coc_size = 10.5;
static const uint  gather_size = 5;
#elif defined(GATHER_HALF_RESOLUTION)
static const float max_coc_size = 21.0;
static const uint  gather_size = 8;
#else
static const float max_coc_size = 42.0;
static const uint  gather_size = 16;
#endif

static const float max_coc_radius = max_coc_size / 2.0;
static const uint  gather_sample_count = gather_size * gather_size;
static const int   flood_fill_radius = 1;
static const float flood_fill_sample_count = (flood_fill_radius * 2 + 1) * (flood_fill_radius * 2 + 1);

#ifndef DOF_NEAR_MASK_TILE_SIZE
#define DOF_NEAR_MASK_TILE_SIZE 16
#endif
#define DOF_NEAR_MASK_THREAD_COUNT DOF_NEAR_MASK_TILE_SIZE * DOF_NEAR_MASK_TILE_SIZE

#ifdef __COMPUTE_SHADER__
groupshared float near_coc_gs[DOF_NEAR_MASK_THREAD_COUNT];
#else
static float near_coc_gs[1];
#endif

struct Output {
   float4 near : SV_Target0;
   float4 far  : SV_Target1;
};

float calc_coc(float camera_distance)
{
   // Unoptimized Reference
#if 0
   float coc = -aperture_diameter * ((focal_length * (focus_distance - camera_distance)) /
                                     (camera_distance * (focus_distance - focal_length)));

   coc /= film_size;
   coc *= target_size.x;
   coc /= max_coc_size;

   return coc;
#endif
   // Optimized calculation, pre-computing multiplies and some divides on the CPU.
   float coc = coc_mul * ((focus_distance - camera_distance) / camera_distance);

   coc /= max_coc_size;

   return coc;
}

Output prepare_ps(float2 vs_texcoords : TEXCOORD, float4 positionSS : SV_Position)
{
   const float camera_distance = proj_from_view_m43 / (depth_input[positionSS.xy] - proj_from_view_m33);
   const float coc = calc_coc(camera_distance);

   const float3 scene_color = scene_input[positionSS.xy];

   const float near_coc = saturate(-coc);
   const float far_coc = saturate(coc);

   Output output;

   output.near = float4(scene_color, near_coc);
   output.far = float4(scene_color * far_coc, far_coc);

   return output;
}

Output prepare_downsample_2x_ps(float2 vs_texcoords : TEXCOORD, float4 positionSS : SV_Position)
{
   const float2 texcoords = positionSS.xy * inv_target_size;

   const float4 depths = depth_input.GatherRed(linear_sample, texcoords);
   const float4 camera_distances = proj_from_view_m43 / (depths - proj_from_view_m33);
   const float coc = (calc_coc(camera_distances[0]) + 
                      calc_coc(camera_distances[1]) + 
                      calc_coc(camera_distances[2]) + 
                      calc_coc(camera_distances[3])) / 4.0f;

   const float3 scene_color = scene_input.Sample(linear_sample, texcoords);

   const float near_coc = saturate(-coc);
   const float far_coc = saturate(coc);

   Output output;

   output.near = float4(scene_color, near_coc);
   output.far = float4(scene_color * far_coc, far_coc);

   return output;
}

Output prepare_downsample_4x_ps(float2 vs_texcoords : TEXCOORD, float4 positionSS : SV_Position)
{
   const float2 texcoords = positionSS.xy * inv_target_size;

   const float4 depths = depth_input.GatherRed(linear_sample, texcoords);
   const float4 camera_distances = proj_from_view_m43 / (depths - proj_from_view_m33);
   const float coc = (calc_coc(camera_distances[0]) + 
                      calc_coc(camera_distances[1]) + 
                      calc_coc(camera_distances[2]) + 
                      calc_coc(camera_distances[3])) / 4.0;

   const float3 scene_color = (scene_input.Sample(linear_sample, texcoords, int2(-1, -1)) +
                              scene_input.Sample(linear_sample, texcoords, int2(1, -1)) +
                              scene_input.Sample(linear_sample, texcoords, int2(-1, 1)) +
                              scene_input.Sample(linear_sample, texcoords, int2(1, 1))) / 4.0;

   const float near_coc = saturate(-coc);
   const float far_coc = saturate(coc);

   Output output;

   output.near = float4(scene_color, near_coc);
   output.far = float4(scene_color * far_coc, far_coc);

   return output;
}

[numthreads(DOF_NEAR_MASK_TILE_SIZE, DOF_NEAR_MASK_TILE_SIZE, 1)]
void downsample_near_mask_cs(uint3 dispatch_id : SV_DispatchThreadID, 
                             uint group_index : SV_GroupIndex, 
                             uint3 group_id : SV_GroupID)
{
   near_coc_gs[group_index] = tile_near_coc_input[dispatch_id.xy].w;

   GroupMemoryBarrierWithGroupSync();

   [unroll]
   for (uint index = DOF_NEAR_MASK_THREAD_COUNT / 2; index > 0; index >>= 1) {
      if (group_index < index) near_coc_gs[group_index] = max(near_coc_gs[group_index], near_coc_gs[group_index + index]);
   
      GroupMemoryBarrierWithGroupSync();
   }

   if (group_index == 0) tile_near_coc_output[group_id.xy] = near_coc_gs[0];
}

float blur_near_mask_ps(float2 texcoords, float2 direction) : SV_Target0
{
#  ifdef BLUR_NEAR_MASK_QUARTER

   const int step_count = 1;
   const float weights[step_count] = {
      0.5
   };
   const float offsets[step_count] = {
      0.65264
   };

#  elif defined(BLUR_NEAR_MASK_HALF)

   const int step_count = 2;
   const float weights[step_count] = {
      0.25814,
      0.24186
   };
   const float offsets[step_count] = {
      0.65264,
      2.42250
   };

#  else

   const int step_count = 4;
   const float weights[step_count] = {
      0.20460,
      0.19169,
      0.08208,
      0.02163
   };
   const float offsets[step_count] = {
      0.65264,
      2.42250,
      4.36297,
      6.30736
   };

#  endif

   float coc = 0.0;

   [unroll]
   for (int i = 0; i < step_count; ++i) {
      coc += 
         (blur_near_mask_input.Sample(linear_sample, texcoords - offsets[i] * direction) + 
          blur_near_mask_input.Sample(linear_sample, texcoords + offsets[i] * direction)) 
         * weights[i];
   }

   return coc;
}

float blur_x_near_mask_ps(float2 vs_texcoords : TEXCOORD, float4 positionSS : SV_Position) : SV_Target0
{
   return max(blur_near_mask_input[positionSS.xy], 
              blur_near_mask_ps(positionSS.xy * inv_near_mask_size, float2(inv_near_mask_size.x, 0.0)));
}

float blur_y_near_mask_ps(float2 vs_texcoords : TEXCOORD, float4 positionSS : SV_Position) : SV_Target0
{
   return max(blur_near_mask_input[positionSS.xy], 
              blur_near_mask_ps(positionSS.xy * inv_near_mask_size, float2(0.0, inv_near_mask_size.y)));
}

float2 concentric_sample_disk(float2 u) 
{
   const static float pi = 3.14159265358979323846;
   const static float pi_over_2 = pi / 2.0;
   const static float pi_over_4 = pi / 4.0;

   const float2 offset = 2.0 * u - 1.0;

   if (offset.x == 0.0 && offset.y == 0.0) return 0.0;

   float theta = 0.0;
   float r = 0.0;

   if (abs(offset.x) > abs(offset.y)) {
      r = offset.x;
      theta = pi_over_4 * (offset.y / offset.x);
   }
   else {
      r = offset.y;
      theta = pi_over_2 - pi_over_4 * (offset.x / offset.y);
   }

   return r * float2(cos(theta), sin(theta));
}

// Bicubic Filtering in Fewer Taps, Phill Djonov - https://vec3.ca/posts/bicubic-filtering-in-fewer-taps
float bicubic_sample_bspline(Texture2D<float> tex, SamplerState samp, float2 texcoord, 
                             float2 tex_size, float2 inv_tex_size)
{
   float2 itexcoord = texcoord * tex_size;
   float2 texel_centre = floor(itexcoord - 0.5) + 0.5;

   float2 f = itexcoord - texel_centre;
   float2 f2 = f * f;
   float2 f3 = f2 * f;
    
   float2 inv_f = 1.0 - f; 
   float2 inv_f2 = inv_f * inv_f;
   float2 inv_f3 = inv_f2 * inv_f;
    
   float2 w0 = (1.0 / 6.0) * inv_f3;
   float2 w1 = (1.0 / 6.0) * (4.0 + 3.0 * f3 - 6.0 * f2);
   float2 w2 = (1.0 / 6.0) * (4.0 + 3.0 * inv_f3 - 6.0 * inv_f2);
   float2 w3 = (1.0 / 6.0) * f3;

   float2 s0 = w0 + w1;
   float2 s1 = w2 + w3;
    
   float2 f0 = w1 / (w0 + w1);
   float2 f1 = w3 / (w2 + w3);

   float2 t0 = texel_centre - 1.0 + f0;
   float2 t1 = texel_centre + 1.0 + f1;

   t0 *= inv_tex_size;
   t1 *= inv_tex_size;

   return   (tex.Sample(samp, float2(t0.x, t0.y)) * s0.x + 
             tex.Sample(samp, float2(t1.x, t0.y)) * s1.x) * s0.y +
            (tex.Sample(samp, float2(t0.x, t1.y)) * s0.x +
             tex.Sample(samp, float2(t1.x, t1.y)) * s1.x) * s1.y;
}

float sample_near_mask(float2 texcoord)
{
#  ifdef GATHER_FAST
   return near_mask_input.Sample(linear_clamp_sample, texcoord);
#  else
   return bicubic_sample_bspline(near_mask_input, linear_clamp_sample, texcoord, 
                                 near_mask_size, inv_near_mask_size);
#  endif

}

Output gather_ps(float2 vs_texcoords : TEXCOORD, float4 positionSS : SV_Position)
{
   float4 far = 0.0;
   const float4 pixel_far = far_input[positionSS.xy];
   const float far_coc = pixel_far.w;
   const float far_radius = max_coc_radius * far_coc;

   [branch]
   if (far_radius > 0.5) {
      [unroll]
      for (uint y = 0; y < gather_size; ++y) {
         [unroll]
         for (uint x = 0; x < gather_size; ++x) {
            float2 sample_offset = concentric_sample_disk(float2(x, y) / (gather_size - 1.0)) * far_radius;
            float4 sample = far_input.Sample(linear_sample, (positionSS.xy + sample_offset) * inv_target_size);
 
            far += sample * saturate(1.0 + (sample.w - far_coc));
         }
      }

      far /= gather_sample_count;
   }
   else {
      far = pixel_far;
   }

   float4 near = 0.0;

   const float4 pixel_near = near_input[positionSS.xy];
   const float pixel_near_coc = pixel_near.w;
   const float near_coc_mask = sample_near_mask(positionSS.xy * inv_target_size);
   const float near_coc = max(pixel_near_coc, near_coc_mask);
   const float near_radius = max_coc_radius * near_coc;

   [branch]
   if (near_radius > 0.5) {
      [unroll]
      for (uint y = 0; y < gather_size; ++y) {
         [unroll]
         for (uint x = 0; x < gather_size; ++x) {
            float2 sample_offset = concentric_sample_disk(float2(x, y) / (gather_size - 1.0)) * near_radius;
            float4 sample = near_input.Sample(linear_sample, (positionSS.xy + sample_offset) * inv_target_size);

            near.rgb += sample.rgb;
            near.w += saturate(sample.w * max_coc_radius);
         }
      }

      near /= gather_sample_count;
      near.w = max(pixel_near_coc, saturate(near.w));
   }
   else {
      near = float4(pixel_near.rgb, 0.0);
   }

   Output output;

   output.near = near;
   output.far = far;

   return output;
}

float4 gather_far_ps(float2 vs_texcoords : TEXCOORD, float4 positionSS : SV_Position) : SV_Target0
{
   float4 far = 0.0;
   const float4 pixel_far = far_input[positionSS.xy];
   const float far_coc = pixel_far.w;
   const float far_radius = max_coc_radius * far_coc;

   [branch]
   if (far_radius > 0.5) {
      [unroll]
      for (uint y = 0; y < gather_size; ++y) {
         [unroll]
         for (uint x = 0; x < gather_size; ++x) {
            float2 sample_offset = concentric_sample_disk(float2(x, y) / (gather_size - 1.0)) * far_radius;
            float4 sample = far_input.Sample(linear_sample, (positionSS.xy + sample_offset) * inv_target_size);
 
            far += sample * saturate(1.0 + (sample.w - far_coc));
         }
      }

      far /= gather_sample_count;
   }
   else {
      far = pixel_far;
   }

   return far;
}

float4 gather_near_ps(float2 vs_texcoords : TEXCOORD, float4 positionSS : SV_Position) : SV_Target0
{
   float4 near = 0.0;

   const float4 pixel_near = near_input[positionSS.xy];
   const float pixel_near_coc = pixel_near.w;
   const float near_coc_mask = sample_near_mask(positionSS.xy * inv_target_size);
   const float near_coc = max(pixel_near_coc, near_coc_mask);
   const float near_radius = max_coc_radius * near_coc;

   [branch]
   if (near_radius > 0.5) {
      [unroll]
      for (uint y = 0; y < gather_size; ++y) {
         [unroll]
         for (uint x = 0; x < gather_size; ++x) {
            float2 sample_offset = concentric_sample_disk(float2(x, y) / (gather_size - 1.0)) * near_radius;
            float4 sample = near_input.Sample(linear_sample, (positionSS.xy + sample_offset) * inv_target_size);

            near.rgb += sample.rgb;
            near.w += saturate(sample.w * max_coc_radius);
         }
      }

      near /= gather_sample_count;
      near.w = max(pixel_near_coc, saturate(near.w));
   }
   else {
      near = float4(pixel_near.rgb, 0.0);
   }

   return near;
}

Output floodfill_ps(float2 vs_texcoords : TEXCOORD, float4 positionSS : SV_Position)
{
   float4 near = 0.0;
   float4 far = 0.0;

   int2 position = positionSS.xy;

   for (int y = -flood_fill_radius; y <= flood_fill_radius; ++y) {
      for (int x = -flood_fill_radius; x <= flood_fill_radius; ++x) {
         near += floodfill_near_input[position + int2(x, y)];
         far += floodfill_far_input[position + int2(x, y)];
      }
   }

   near /= flood_fill_sample_count;
   far /= flood_fill_sample_count;
   
   Output output;

   output.near = near;
   output.far = far;

   return output;
}

float4 compose_ps(float2 texcoords : TEXCOORD, float4 positionSS : SV_Position) : SV_Target0
{
   int2 position = positionSS.xy;

   float3 scene = compose_input[positionSS.xy];
   float4 near = compose_near_input[positionSS.xy];
   float4 far = compose_far_input[positionSS.xy];
   
   const float camera_distance = proj_from_view_m43 / (compose_depth_input[positionSS.xy] - proj_from_view_m33);
   const float coc = calc_coc(camera_distance);
   const float far_coc = saturate(coc);

   if (far.w > 0.0) far.rgb /= far.w;

   float far_blend = saturate(far_coc * max_coc_size - 0.5);

   scene = lerp(scene, far.rgb, smoothstep(0.0, 1.0, far_blend));

   float near_blend = saturate(near.w * 2.0);

   scene = lerp(scene, near.rgb, smoothstep(0.0, 1.0, near_blend));

   return float4(scene, 1.0);
}

float4 compose_upsample_2x_ps(float2 texcoords : TEXCOORD, float4 positionSS : SV_Position) : SV_Target0
{
   int2 position = positionSS.xy;

   float3 scene = compose_input[positionSS.xy];
   float4 near = compose_near_input.Sample(linear_sample, texcoords);
   float4 far = compose_far_input.Sample(linear_sample, texcoords);
                         
   const float camera_distance = proj_from_view_m43 / (compose_depth_input[positionSS.xy] - proj_from_view_m33);
   const float coc = calc_coc(camera_distance);
   const float far_coc = saturate(coc);

   if (far.w > 0.0) far.rgb /= far.w;

   float far_blend = saturate(far_coc * max_coc_size - 0.5);

   scene = lerp(scene, far.rgb, smoothstep(0.0, 1.0, far_blend));

   float near_blend = saturate(near.w * 2.0);

   scene = lerp(scene, near.rgb, smoothstep(0.0, 1.0, near_blend));

   return float4(scene, 1.0);
}

float4 bicubic_sample_bspline(Texture2D tex, SamplerState samp, float2 texcoord, 
                              float2 tex_size, float2 inv_tex_size)
{
   float2 itexcoord = texcoord * tex_size;
   float2 texel_centre = floor(itexcoord - 0.5) + 0.5;

   float2 f = itexcoord - texel_centre;
   float2 f2 = f * f;
   float2 f3 = f2 * f;
    
   float2 inv_f = 1.0 - f; 
   float2 inv_f2 = inv_f * inv_f;
   float2 inv_f3 = inv_f2 * inv_f;
    
   float2 w0 = (1.0 / 6.0) * inv_f3;
   float2 w1 = (1.0 / 6.0) * (4.0 + 3.0 * f3 - 6.0 * f2);
   float2 w2 = (1.0 / 6.0) * (4.0 + 3.0 * inv_f3 - 6.0 * inv_f2);
   float2 w3 = (1.0 / 6.0) * f3;

   float2 s0 = w0 + w1;
   float2 s1 = w2 + w3;
    
   float2 f0 = w1 / (w0 + w1);
   float2 f1 = w3 / (w2 + w3);

   float2 t0 = texel_centre - 1.0 + f0;
   float2 t1 = texel_centre + 1.0 + f1;

   t0 *= inv_tex_size;
   t1 *= inv_tex_size;

   return   (tex.Sample(samp, float2(t0.x, t0.y)) * s0.x + 
             tex.Sample(samp, float2(t1.x, t0.y)) * s1.x) * s0.y +
            (tex.Sample(samp, float2(t0.x, t1.y)) * s0.x +
             tex.Sample(samp, float2(t1.x, t1.y)) * s1.x) * s1.y;
}

float4 compose_upsample_4x_ps(float2 texcoords : TEXCOORD, float4 positionSS : SV_Position) : SV_Target0
{
   int2 position = positionSS.xy;

   float3 scene = compose_input[positionSS.xy];

#  ifdef COMPOSE_FAST
   float4 near = compose_near_input.Sample( linear_clamp_sample, texcoords);
   float4 far = compose_far_input.Sample(linear_clamp_sample, texcoords);
#  else
   float4 near = bicubic_sample_bspline(compose_near_input, linear_clamp_sample, texcoords, 
                                        target_size, inv_target_size);
   float4 far = bicubic_sample_bspline(compose_far_input, linear_clamp_sample, texcoords, 
                                       target_size, inv_target_size);
#endif

   const float camera_distance = proj_from_view_m43 / (compose_depth_input[positionSS.xy] - proj_from_view_m33);
   const float coc = calc_coc(camera_distance);
   const float far_coc = saturate(coc);

   if (far.w > 0.0) far.rgb /= far.w;

   float far_blend = saturate(far_coc * max_coc_size - 0.5);

   scene = lerp(scene, far.rgb, smoothstep(0.0, 1.0, far_blend));

   float near_blend = saturate(near.w * 2.0);

   scene = lerp(scene, near.rgb, smoothstep(0.0, 1.0, near_blend));

   return float4(scene, 1.0);
}