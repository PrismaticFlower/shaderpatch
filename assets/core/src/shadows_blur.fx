
float4 main_vs(float2 position : POSITION, inout float2 texcoords : TEXCOORD,
               uniform float2 pixel_offset : register(c110)) : POSITION
{
   return float4(position + pixel_offset, 0.0, 1.0);
}

sampler2D buffer_sampler : register(s4);
sampler2D depth_sampler : register(s5);

static const float blur_radius = BLUR_RADIUS;
static const float blur_sigma = blur_radius * 0.5;
static const float blur_falloff = 1.0 / (2.0 * blur_sigma * blur_sigma);
float2 direction_inv_res : register(c60);
float depth_sharpness : register(c61);

float blur_function(float2 texcoords, float r, float centre_v, float centre_d, inout float w_total)
{
   float v = tex2D(buffer_sampler, texcoords).a;
   float d = tex2D(depth_sampler, texcoords).r;

   float d_diff = (d - centre_d);
   float w = exp2(-r * r * blur_falloff - d_diff * d_diff);
   w_total += w;
   
   return v * w;
}

float4 blur_ps(float2 texcoords : TEXCOORD) : COLOR
{
   float centre_v = tex2D(buffer_sampler, texcoords).a;
   float centre_d = tex2D(depth_sampler, texcoords).r;
   
   float v_total = centre_v;
   float w_total = 1.0;
   
   [unroll] for (float r = 1; r <= blur_radius; ++r) {
      v_total += blur_function(texcoords + (direction_inv_res * r), r,
         centre_v, centre_d, w_total);
      v_total += blur_function(texcoords - (direction_inv_res * r), r,
                               centre_v, centre_d, w_total);
   }
   
   return v_total / w_total;
}