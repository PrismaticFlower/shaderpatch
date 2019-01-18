
#include "generic_vertex_input.hlsl"
#include "vertex_utilities.hlsl"
#include "vertex_transformer.hlsl"
#include "pixel_utilities.hlsl"
#include "pixel_sampler_states.hlsl"
#include "lighting_utilities.hlsl"

const static float3 specular_color = custom_constants[0].xyz;
const static float4 shield_constants = custom_constants[3]; 
const static float2 near_scene_fade_scale = custom_constants[2].xy;

Texture2D<float4> diffuse_texture : register(t0);
Texture2D<float2> normal_map_texture : register(t4);
Texture2D<float3> refraction_buffer : register(t5);

float3 animate_normal(float3 normal)
{
   float x_angle = lerp(0, 6.283185, shield_constants.y);
   float y_angle = lerp(0, 6.283185, shield_constants.x);

   float3x3 x_trans = {{cos(x_angle), -sin(x_angle), 0.0},
                       {sin(x_angle), cos(x_angle), 0.0},
                       {0.0, 0.0, 1.0}};

   normal = mul(x_trans, normal);

   float3x3 y_trans = {{cos(y_angle), 0.0, sin(y_angle)},
                       {0.0, 1.0, 0.0},
                       {-sin(y_angle), 0.0, cos(y_angle)}};

   return mul(y_trans, normal);
}

float angle_factor(float3 view_normal, float3 normal)
{
   float3 reflected_view_normal = reflect(view_normal, normal);

   float view_angle = dot(reflected_view_normal, reflected_view_normal);
   view_angle = max(view_angle, shield_constants.w);

   float factor = rcp(view_angle);
   view_angle = dot(view_normal, reflected_view_normal);
   factor *= view_angle;
   factor = factor * -0.5 + 0.5;
   factor *= max(shield_constants.z, 1.0);
   factor = -factor * material_diffuse_color.a + material_diffuse_color.a;

   return factor;
}

struct Vs_output
{
   float2 texcoords : TEXCOORD0;
   float3 normal_texcoords : TEXCOORD1;
   float3 normalWS : NORMALWS;
   float3 view_normalWS : VIEWNORMALWS;
   float3 material_color : MATCOLOR;
   float fog : FOG;
   float alpha : ALPHA;

   float4 positionPS : SV_Position;
};

Vs_output shield_vs(Vertex_input input)
{
   Vs_output output;

   Transformer transformer = create_transformer(input);

   const float3 positionWS = transformer.positionWS();
   const float4 positionPS = transformer.positionPS();
   const float3 normalOS = normalize(-transformer.positionOS());
   const float3 normalWS = mul(normalOS, (float3x3)world_matrix);
   const float3 view_normalWS = normalize(positionWS - view_positionWS);

   output.positionPS = positionPS;
   output.normalWS = normalWS;
   output.view_normalWS = view_normalWS;

   output.texcoords = input.texcoords() + shield_constants.xy;
   output.normal_texcoords = animate_normal(normalWS);

   float near_fade;
   calculate_near_fade_and_fog(positionWS, positionPS, near_fade, output.fog);
   near_fade = near_fade * near_scene_fade_scale.x + near_scene_fade_scale.y;

   output.material_color = get_material_color().rgb;
   output.alpha = saturate(angle_factor(view_normalWS, normalWS) * near_fade);

   return output;
}

float2 map_xyz_to_uv(float3 pos)
{
   float3 abs_pos = abs(pos);
   float3 pos_positive = sign(pos) * 0.5 + 0.5;

   float max_axis;
   float2 coords;

   if (abs_pos.x >= abs_pos.y && abs_pos.x >= abs_pos.z) {
      max_axis = abs_pos.x;

      coords = lerp(float2(pos.z, pos.y), float2(-pos.z, pos.y), pos_positive.x);
   }
   else if (abs_pos.y >= abs_pos.z) {
      max_axis = abs_pos.y;

      coords = lerp(float2(pos.x, pos.z), float2(pos.x, -pos.z), pos_positive.y);
   }
   else {
      max_axis = abs_pos.z;

      coords = lerp(float2(-pos.x, pos.y), float2(pos.x, pos.y), pos_positive.z);
   }

   return 0.5 * (coords / max_axis + 1.0);
}

struct Ps_input
{
   float2 texcoords : TEXCOORD0;
   float3 normal_texcoords : TEXCOORD1;
   float3 normalWS : NORMALWS;
   float3 view_normalWS : VIEWNORMALWS;
   float3 material_color : MATCOLOR;
   float fog : FOG;
   float alpha : ALPHA;
   float4 positionSS : SV_Position;
};

float4 shield_ps(Ps_input input) : SV_Target0
{
   const float3 view_normalWS = normalize(input.view_normalWS);
   const float3 normalWS = perturb_normal(normal_map_texture, aniso_wrap_sampler,
                                          map_xyz_to_uv(input.normal_texcoords),
                                          normalize(input.normalWS), view_normalWS);

   const float2 scene_coords = input.positionSS.xy * rt_resolution.zw + normalWS.xz * 0.1;
   const float3 scene_color = refraction_buffer.SampleLevel(linear_clamp_sampler, scene_coords, 0.0);
   const float4 diffuse_color = diffuse_texture.Sample(linear_clamp_sampler, input.texcoords);

   const float3 H = normalize(light_directional_0_dir.xyz + view_normalWS);
   const float NdotH = saturate(dot(normalWS, H));
   float3 specular = pow(NdotH, 64);
   specular *= specular_color.rgb;

   float3 color = diffuse_color.rgb * input.material_color;

   float alpha = material_diffuse_color.a * diffuse_color.a;

   color = (color * alpha + specular) * lighting_scale;

   color = apply_fog(color, input.fog);

   return float4(color + scene_color, input.alpha);
}
