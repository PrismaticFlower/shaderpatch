
#include "constants_list.hlsl"
#include "generic_vertex_input.hlsl"
#include "lighting_utilities.hlsl"
#include "pixel_sampler_states.hlsl"
#include "pixel_utilities.hlsl"
#include "vertex_transformer.hlsl"
#include "vertex_utilities.hlsl"

const static float4 x_texcoords_transform = custom_constants[0];
const static float4 y_texcoords_transform = custom_constants[1];

const static float3 light_colors[3] = {ps_custom_constants[0].xyz,
                                       ps_custom_constants[1].xyz,
                                       ps_custom_constants[2].xyz};

const static float4 light_positionsWS[3] = {light_packed_constants[0],
                                            light_packed_constants[2],
                                            light_packed_constants[4]};
const static float light_radiuses[3] =
   {(1.0 / light_packed_constants[1].x) * light_packed_constants[1].y,
    (1.0 / light_packed_constants[3].x) * light_packed_constants[3].y,
    (1.0 / light_packed_constants[5].x) * light_packed_constants[5].y};

const static float3 spotlight_color = light_colors[0];
const static float3 spotlight_positionWS = light_packed_constants[0].xyz;
const static float spotlight_range =
   (1.0 / light_packed_constants[1].x) * light_packed_constants[1].y;
const static float3 spotlight_directionWS = light_packed_constants[2].xyz;

const static bool generate_texcoords = PERPIXEL_GENERATE_TEXCOORDS;
const static bool generate_tangents = PERPIXEL_GENERATE_TANGENTS;
const static uint light_count = PERPIXEL_LIGHT_COUNT;

Texture2D<float3> normal_map : register(t0);
Texture2D<float3> projection_map : register(t2);
Texture2D<float2> shadow_ao_map : register(t5);

struct Vs_perpixel_output {
   float3 positionWS : POSITIONWS;
   float3 normalWS : NORMALWS;
   float3 static_lighting : STATICLIGHT;
   float1 fog : FOG;
   noperspective float2 shadow_texcoords : SHADOWTEXCOORDS;

   float4 positionPS : SV_Position;
};

Vs_perpixel_output perpixel_vs(Vertex_input input)
{
   Vs_perpixel_output output;

   Transformer transformer = create_transformer(input);

   const float3 positionWS = transformer.positionWS();
   const float4 positionPS = transformer.positionPS();

   output.positionWS = positionWS;
   output.positionPS = positionPS;
   output.normalWS = transformer.normalWS();
   output.static_lighting = get_static_diffuse_color(input.color());
   output.fog = calculate_fog(positionWS, positionPS);
   output.shadow_texcoords = transform_shadowmap_coords(positionPS);

   return output;
}

struct Vs_normalmapped_output {
   float3 positionWS : POSITIONWS;
   float2 texcoords : TEXCOORD;
   float3 static_lighting : STATICLIGHT;
   float1 fog : FOG;
   noperspective float2 shadow_texcoords : SHADOWTEXCOORDS;

   float3x3 TBN : TBN;

   float4 positionPS : SV_Position;
};

Vs_normalmapped_output normalmapped_vs(Vertex_input input)
{
   Vs_normalmapped_output output;

   Transformer transformer = create_transformer(input);

   const float3 positionWS = transformer.positionWS();
   const float4 positionPS = transformer.positionPS();

   output.positionWS = positionWS;
   output.positionPS = positionPS;

   if (generate_texcoords) {
      output.texcoords.x = dot(float4(positionWS, 1.0), x_texcoords_transform.xzyw);
      output.texcoords.y = dot(float4(positionWS, 1.0), y_texcoords_transform.xzyw);
   }
   else {
      output.texcoords =
         transformer.texcoords(x_texcoords_transform, y_texcoords_transform);
   }

   const float3 normalWS = transformer.normalWS();
   float3 tangentWS;
   float3 bitangentWS;

   if (generate_tangents) {
      generate_terrain_tangents(normalWS, tangentWS, bitangentWS);
   }
   else {
      tangentWS = transformer.tangentWS();
      bitangentWS = transformer.bitangentWS();
   }

   const float3x3 TBN = {tangentWS, bitangentWS, normalWS};

   output.TBN = transpose(TBN);

   output.static_lighting = get_static_diffuse_color(input.color());
   output.fog = calculate_fog(positionWS, positionPS);
   output.shadow_texcoords = transform_shadowmap_coords(positionPS);

   return output;
}

float4 transform_spotlight_projection(float3 positionWS)
{
   float4 projection_coords;

   projection_coords.x = dot(float4(positionWS, 1.0), light_packed_constants[3]);
   projection_coords.y = dot(float4(positionWS, 1.0), light_packed_constants[4]);
   projection_coords.z = dot(float4(positionWS, 1.0), light_packed_constants[5]);
   projection_coords.w = dot(float4(positionWS, 1.0), light_packed_constants[6]);

   return projection_coords;
}

struct Vs_perpixel_spotlight_output {
   float3 positionWS : POSITIONWS;
   float3 normalWS : NORMALWS;
   float4 projection_coords : TEXCOORD;
   float3 static_lighting : STATICLIGHT;
   float1 fog : FOG;
   noperspective float2 shadow_texcoords : SHADOWTEXCOORDS;

   float4 positionPS : SV_Position;
};

Vs_perpixel_spotlight_output perpixel_spotlight_vs(Vertex_input input)
{
   Vs_perpixel_spotlight_output output;

   Transformer transformer = create_transformer(input);

   const float3 positionWS = transformer.positionWS();
   const float4 positionPS = transformer.positionPS();

   output.positionWS = positionWS;
   output.positionPS = positionPS;
   output.normalWS = transformer.normalWS();
   output.projection_coords = transform_spotlight_projection(positionWS);
   output.static_lighting = get_static_diffuse_color(input.color());
   output.fog = calculate_fog(positionWS, positionPS);
   output.shadow_texcoords = transform_shadowmap_coords(positionPS);

   return output;
}

struct Vs_normalmapped_spotlight_output {
   float3 positionWS : POSITIONWS;
   float2 texcoords : TEXCOORD0;
   float4 projection_coords : TEXCOORD1;
   float3 static_lighting : STATICLIGHT;
   float1 fog : FOG;
   noperspective float2 shadow_texcoords : SHADOWTEXCOORDS;

   float3x3 TBN : TBN;

   float4 positionPS : SV_Position;
};

Vs_normalmapped_spotlight_output normalmapped_spotlight_vs(Vertex_input input)
{
   Vs_normalmapped_spotlight_output output;

   Transformer transformer = create_transformer(input);

   const float3 positionWS = transformer.positionWS();
   const float4 positionPS = transformer.positionPS();

   output.positionPS = transformer.positionPS();
   output.positionWS = positionWS;

   if (generate_texcoords) {
      output.texcoords.x = dot(float4(positionWS, 1.0), x_texcoords_transform.xzyw);
      output.texcoords.y = dot(float4(positionWS, 1.0), y_texcoords_transform.xzyw);
   }
   else {
      output.texcoords =
         transformer.texcoords(x_texcoords_transform, y_texcoords_transform);
   }

   output.projection_coords = transform_spotlight_projection(positionWS);

   const float3 normalWS = transformer.normalWS();
   float3 tangentWS;
   float3 bitangentWS;

   if (generate_tangents) {
      generate_terrain_tangents(normalWS, tangentWS, bitangentWS);
   }
   else {
      tangentWS = transformer.tangentWS();
      bitangentWS = transformer.bitangentWS();
   }

   const float3x3 TBN = {tangentWS, bitangentWS, normalWS};

   output.TBN = transpose(TBN);

   output.static_lighting = get_static_diffuse_color(input.color());
   output.fog = calculate_fog(positionWS, positionPS);
   output.shadow_texcoords = transform_shadowmap_coords(positionPS);

   return output;
}

float3 calculate_light(float3 position, float3 normal, float4 light_position,
                       float3 light_color, float light_radius)
{
   float3 light_direction;
   float attenuation;

   if (light_position.w > 0) {
      light_direction = normalize(light_position.xyz - position);

      const float dst = distance(light_position.xyz, position);

      attenuation = saturate(1.0 - (dst * dst) / (light_radius * light_radius));
   }
   else {
      light_direction = -light_position.xyz;
      attenuation = 1.0;
   }

   float intensity = saturate(dot(normal, light_direction));

   return attenuation * intensity * light_color;
}

struct Ps_normalmapped_input {
   float3 positionWS : POSITIONWS;
   float2 texcoords : TEXCOORD;
   float3 static_lighting : STATICLIGHT;
   float1 fog : FOG;
   noperspective float2 shadow_texcoords : SHADOWTEXCOORDS;

   float3x3 TBN : TBN;
};

float4 normalmapped_ps(Ps_normalmapped_input input) : SV_Target0
{
   const float3 normalTS =
      normal_map.Sample(aniso_wrap_sampler, input.texcoords).rgb * (255.0 / 127.0) -
      (128.0 / 127.0);

   const float3 normalWS = normalize(mul(input.TBN, normalTS));

   float3 color = input.static_lighting;
   color += light::ambient(normalWS);
   color *= light_ambient_color_top.a;
   [branch] if (ssao_enabled) color *= shadow_ao_map.Sample(linear_clamp_sampler, input.shadow_texcoords).g;

   [unroll] for (uint i = 0; i < light_count; ++i)
   {
      color += calculate_light(input.positionWS, normalWS, light_positionsWS[i],
                               light_colors[i], light_radiuses[i]);
   }

   color = apply_fog(color, input.fog);

   return float4(color, 1.0);
}

struct Ps_perpixel_input {
   float3 positionWS : POSITIONWS;
   float3 normalWS : NORMALWS;
   float3 static_lighting : STATICLIGHT;
   float1 fog : FOG;
   noperspective float2 shadow_texcoords : SHADOWTEXCOORDS;
};

float4 perpixel_ps(Ps_perpixel_input input) : SV_Target0
{
   const float3 normalWS = normalize(input.normalWS);

   float3 color = input.static_lighting;
   color += light::ambient(normalWS);
   color *= light_ambient_color_top.a;
   [branch] if (ssao_enabled) color *= shadow_ao_map.Sample(linear_clamp_sampler, input.shadow_texcoords).g;

   [unroll] for (uint i = 0; i < light_count; ++i)
   {
      color += calculate_light(input.positionWS, normalWS, light_positionsWS[i],
                               light_colors[i], light_radiuses[i]);
   }

   color = apply_fog(color, input.fog);

   return float4(color, 1.0);
}

float3 calculate_spotlight(float3 position, float3 normal, float3 light_position,
                           float3 light_direction, float3 light_color,
                           float light_range, float3 projection_color)
{
   const float3 light_normal = normalize(light_position - position);
   const float intensity = saturate(dot(normal, light_normal));

   const float light_distance = distance(position, light_position);

   const float attenuation =
      saturate(saturate(dot(light_direction, -light_normal)) -
               ((light_distance * light_distance) / (light_range * light_range)));

   return attenuation * intensity * light_color * projection_color;
}

struct Ps_perpixel_spotlight_input {
   float3 positionWS : POSITIONWS;
   float3 normalWS : NORMALWS;
   float4 projection_coords : TEXCOORD;
   float3 static_lighting : STATICLIGHT;
   float1 fog : FOG;
   noperspective float2 shadow_texcoords : SHADOWTEXCOORDS;
};

float4 perpixel_spotlight_ps(Ps_perpixel_spotlight_input input) : SV_Target0
{
   const float3 projection_color =
      sample_projected_light(projection_map, input.projection_coords);

   const float3 normalWS = normalize(input.normalWS);

   float3 color = input.static_lighting;
   color += light::ambient(normalWS);
   color *= light_ambient_color_top.a;
   [branch] if (ssao_enabled) color *= shadow_ao_map.Sample(linear_clamp_sampler, input.shadow_texcoords).g;

   color += calculate_spotlight(input.positionWS, normalWS, spotlight_positionWS,
                                spotlight_directionWS, spotlight_color,
                                spotlight_range, projection_color);

   color = apply_fog(color, input.fog);

   return float4(color, 1.0);
}

struct Ps_normalmapped_spotlight_input {
   float3 positionWS : POSITIONWS;
   float2 texcoords : TEXCOORD0;
   float4 projection_coords : TEXCOORD1;
   float3 static_lighting : STATICLIGHT;
   float1 fog : FOG;
   noperspective float2 shadow_texcoords : SHADOWTEXCOORDS;

   float3x3 TBN : TBN;
};

float4 normalmapped_spotlight_ps(Ps_normalmapped_spotlight_input input) : SV_Target0
{
   const float3 projection_color =
      sample_projected_light(projection_map, input.projection_coords);

   const float3 normalTS =
      normal_map.Sample(aniso_wrap_sampler, input.texcoords) * (255.0 / 127.0) -
      (128.0 / 127.0);

   const float3 normalWS = normalize(mul(input.TBN, normalTS));

   float3 color = input.static_lighting;
   color += light::ambient(normalWS);
   color *= light_ambient_color_top.a;
   [branch] if (ssao_enabled) color *= shadow_ao_map.Sample(linear_clamp_sampler, input.shadow_texcoords).g;

   color += calculate_spotlight(input.positionWS, normalWS, spotlight_positionWS,
                                spotlight_directionWS, spotlight_color,
                                spotlight_range, projection_color);

   color = apply_fog(color, input.fog);

   return float4(color, 1.0);
}

// Advanced Lighting Shaders

float4 al_normalmapped_ps(Ps_normalmapped_input input, float4 positionSS : SV_Position) : SV_Target0
{
   if (light_positionsWS[0].w != 0) discard;

   const float3 normalTS =
      normal_map.Sample(aniso_wrap_sampler, input.texcoords).rgb * (255.0 / 127.0) -
      (128.0 / 127.0);

   const float3 normalWS = normalize(mul(input.TBN, normalTS));
   const float3 positionWS = input.positionWS;

   float3 color = input.static_lighting;
   color += light::ambient(normalWS);
   color *= light_ambient_color_top.a;
   [branch] if (ssao_enabled) color *= shadow_ao_map.Sample(linear_clamp_sampler, input.shadow_texcoords).g;

   const float3 global_light_directionWS = -light_positionsWS[0].xyz;
   float global_light = saturate(dot(normalWS, global_light_directionWS));

   [branch]
   if (directional_light_0_has_shadow) {
      global_light *= sample_sun_shadow_map(positionWS);
   }

   color += global_light * light_colors[0];

   const light::Cluster_index cluster = light::load_cluster(positionWS, positionSS);

   for (uint i = cluster.point_lights_start; i < cluster.point_lights_end; ++i) {
      light::Cluster_point_light point_light = light::light_clusters_point_lights[light::light_clusters_lists[i]];

      const float3 unorma_light_dirWS = point_light.positionWS - positionWS;
      const float3 light_dirWS = normalize(unorma_light_dirWS);
      const float attenuation = light::attenuation_point(unorma_light_dirWS, point_light.inv_range_sq);
      const float diffuse = saturate(dot(normalWS, light_dirWS));

      color += attenuation * diffuse * point_light.color;
   }
   
   for (uint i = cluster.spot_lights_start; i < cluster.spot_lights_end; ++i) {
      light::Cluster_spot_light spot_light = light::light_clusters_spot_lights[light::light_clusters_lists[i]];
      
      const float3 unorma_light_dirWS = spot_light.positionWS - positionWS;
      const float3 light_dirWS = normalize(unorma_light_dirWS);
      const float attenuation = light::attenuation_point(unorma_light_dirWS, spot_light.inv_range_sq);
      const float diffuse = saturate(dot(normalWS, light_dirWS));

      const float theta = max(dot(-light_dirWS, spot_light.directionWS), 0.0);
      const float cone_falloff = saturate((theta - spot_light.cone_outer_param) * spot_light.cone_inner_param);

      color += attenuation * cone_falloff * diffuse * spot_light.color;
   }

   color = apply_fog(color, input.fog);

   return float4(color, 1.0);
}

float4 al_perpixel_ps(Ps_perpixel_input input, float4 positionSS : SV_Position) : SV_Target0
{
   if (light_positionsWS[0].w != 0) discard;

   const float3 normalWS = normalize(input.normalWS);
   const float3 positionWS = input.positionWS;

   float3 color = input.static_lighting;
   color += light::ambient(normalWS);
   color *= light_ambient_color_top.a;
   [branch] if (ssao_enabled) color *= shadow_ao_map.Sample(linear_clamp_sampler, input.shadow_texcoords).g;

   const float3 global_light_directionWS = -light_positionsWS[0].xyz;
   float global_light = saturate(dot(normalWS, global_light_directionWS));

   [branch]
   if (directional_light_0_has_shadow) {
      global_light *= sample_sun_shadow_map(positionWS);
   }

   color += global_light * light_colors[0];

   const light::Cluster_index cluster = light::load_cluster(positionWS, positionSS);

   for (uint i = cluster.point_lights_start; i < cluster.point_lights_end; ++i) {
      light::Cluster_point_light point_light = light::light_clusters_point_lights[light::light_clusters_lists[i]];

      const float3 unorma_light_dirWS = point_light.positionWS - positionWS;
      const float3 light_dirWS = normalize(unorma_light_dirWS);
      const float attenuation = light::attenuation_point(unorma_light_dirWS, point_light.inv_range_sq);
      const float diffuse = saturate(dot(normalWS, light_dirWS));

      color += attenuation * diffuse * point_light.color;
   }
   
   for (uint i = cluster.spot_lights_start; i < cluster.spot_lights_end; ++i) {
      light::Cluster_spot_light spot_light = light::light_clusters_spot_lights[light::light_clusters_lists[i]];
      
      const float3 unorma_light_dirWS = spot_light.positionWS - positionWS;
      const float3 light_dirWS = normalize(unorma_light_dirWS);
      const float attenuation = light::attenuation_point(unorma_light_dirWS, spot_light.inv_range_sq);
      const float diffuse = saturate(dot(normalWS, light_dirWS));

      const float theta = max(dot(-light_dirWS, spot_light.directionWS), 0.0);
      const float cone_falloff = saturate((theta - spot_light.cone_outer_param) * spot_light.cone_inner_param);

      color += attenuation * cone_falloff * diffuse * spot_light.color;
   }

   color = apply_fog(color, input.fog);

   return float4(color, 1.0);
}

void discard_ps() {}
