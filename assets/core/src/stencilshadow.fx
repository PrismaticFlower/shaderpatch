
#include "constants_list.hlsl"
#include "vertex_utilities.hlsl"
#include "stencilshadow_transform_utilities.hlsl"

const static float3 debug_color = {0.6, 0.2, 0.6};

const static float3 light_directionWS = custom_constants[0].xyz;
const static float2 extrusion_info = custom_constants[1].xy;

struct Vs_extend_input
{
   int4 position : POSITION;
   unorm float4 normal : NORMAL;
};

struct Vs_extend_hardskin_input
{
   int4 position : POSITION;
   unorm float4 normal : NORMAL;
   unorm float4 blend_indices : BLENDINDICES;
};

struct Vs_extend_hardskin_gen_normal_input
{
   int4 position0 : POSITION0;
   unorm float4 blend_indices : BLENDINDICES;
   int4 position1 : POSITION1;
   int4 position2 : POSITION2;
};

struct Vs_extend_softskin_facenormal_input
{
   int4 position : POSITION0;
   unorm float4 blend_weights : BLENDWEIGHT0;
   unorm float4 blend_indices : BLENDINDICES0;
   unorm float4 face_normal : NORMAL;
   unorm float4 face_blend_weights : BLENDWEIGHT1;
   unorm float4 face_blend_indices : BLENDINDICES1;
};

struct Vs_extend_softskin_gen_normal_input
{
   int4 position0 : POSITION0;
   unorm float4 blend_weights0 : BLENDWEIGHT0;
   unorm float4 blend_indices0 : BLENDINDICES0;
   int4 position1 : POSITION1;
   unorm float4 blend_weights1 : BLENDWEIGHT1;
   unorm float4 blend_indices1 : BLENDINDICES1;
   int4 position2 : POSITION2;
   unorm float4 blend_weights2 : BLENDWEIGHT2;
   unorm float4 blend_indices2 : BLENDINDICES2;
};

float4 preextended_vs(float3 position : POSITION) : SV_Position
{
   const float3 positionWS = mul(float4(position, 1.0), world_matrix);

   return mul(float4(positionWS, 1.0), projection_matrix);
}

float3 extrude_directional(float3 position, float3 normal,
                           float3 light_direction, float2 extrusion_info)
{
   float NdotL = dot(normal, -light_direction);

   if (NdotL >= 0.0) NdotL = 1.0f;
   else NdotL = 0.0f;

   const float extrusion_length = extrusion_info.x;
   const float extrusion_offset = extrusion_info.y;

   NdotL = NdotL * extrusion_length + extrusion_offset;

   return NdotL * light_direction + position;
}

float4 extend_directional_vs(Vs_extend_input input) : SV_Position
{
   const float3 positionWS = unskinned_positionWS(input.position);
   const float3 normalWS = unskinned_normalWS(input.normal);

   const float3 positionExtrudedWS = extrude_directional(positionWS, normalWS, 
                                                         light_directionWS,
                                                         extrusion_info);

   return mul(float4(positionExtrudedWS, 1.0), projection_matrix);
}

float4 extend_directional_hardskin_facenormal_vs(Vs_extend_hardskin_input input) : SV_Position
{
   const int4 blend_indices = input.blend_indices * 255.0;
   const float3 positionWS = hard_skinned_positionWS(input.position, blend_indices.x);
   const float3 normalWS = hard_skinned_normalWS(input.normal, (int)input.blend_indices.y);

   const float3 positionExtrudedWS = extrude_directional(positionWS, normalWS,
                                                         light_directionWS,
                                                         extrusion_info);

   return mul(float4(positionExtrudedWS, 1.0), projection_matrix);
}

float4 extend_directional_hardskin_gen_normal_vs(Vs_extend_hardskin_gen_normal_input input) : SV_Position
{
   const int4 blend_indices = input.blend_indices * 255.0;

   const float3 position0WS = hard_skinned_positionWS(input.position0, blend_indices.x);
   const float3 position1WS = hard_skinned_positionWS(input.position1, blend_indices.y);
   const float3 position2WS = hard_skinned_positionWS(input.position2, blend_indices.z);

   float3 normalWS = cross(position2WS - position1WS, position0WS - position1WS);
   normalWS = normalize(normalWS);
   
   const float3 positionExtrudedWS = extrude_directional(position0WS, normalWS,
                                                         light_directionWS,
                                                         extrusion_info);
   
   return mul(float4(positionExtrudedWS, 1.0), projection_matrix);
}

float4 extend_directional_softskin_facenormal_vs(Vs_extend_softskin_facenormal_input input) : SV_Position
{
   const int4 blend_indices = input.blend_indices * 255.0;
   const float3 positionWS = soft_skinned_positionWS(input.position, blend_indices, 
                                                     input.blend_weights);
   
   const int4 face_blend_indices = input.face_blend_indices * 255.0;
   const float3 normalWS = soft_skinned_normalWS(input.face_normal, face_blend_indices,
                                                 input.face_blend_indices);

   const float3 positionExtrudedWS = extrude_directional(positionWS, normalWS,
                                                         light_directionWS,
                                                         extrusion_info);

   return mul(float4(positionExtrudedWS, 1.0), projection_matrix);
}

float4 extend_directional_softskin_gen_normal_vs(Vs_extend_softskin_gen_normal_input input) : SV_Position
{
   const int4 blend_indices[3] = {input.blend_indices0 * 255.0, 
                                  input.blend_indices1 * 255.0,
                                  input.blend_indices2 * 255.0};

   const float3 position0WS = soft_skinned_positionWS(input.position0, input.blend_weights0,
                                                      blend_indices[0]);
   const float3 position1WS = soft_skinned_positionWS(input.position1, input.blend_weights1,
                                                      blend_indices[1]);
   const float3 position2WS = soft_skinned_positionWS(input.position2, input.blend_weights2,
                                                      blend_indices[2]);

   float3 normalWS = cross(position2WS - position1WS, position0WS - position1WS);
   normalWS = normalize(normalWS);

   const float3 positionExtrudedWS = extrude_directional(position0WS, normalWS,
                                                         light_directionWS,
                                                         extrusion_info);

   return mul(float4(positionExtrudedWS, 1.0), projection_matrix);
}

const static float4 light_positionWS = custom_constants[0];
const static float extrusion_offset = custom_constants[1].x;

float3 extrude_point(float3 position, float3 normal, float3 light_position,
                     float light_radius, float extrusion_offset)
{
   const float3 light_direction = normalize(position - light_position.xyz);
   const float LdotL = dot(light_direction, light_direction);
   const float extrude_amount = max(light_radius - LdotL, 0.0);

   float NdotL = dot(normal, -light_direction);

   NdotL = (NdotL >= 0.0) ? 1.0 : 0.0;
   NdotL = NdotL * extrude_amount + extrusion_offset;

   return NdotL * light_direction + position;
}

float4 extend_point_vs(Vs_extend_input input) : SV_Position
{
   const float3 positionWS = unskinned_positionWS(input.position);
   const float3 normalWS = unskinned_normalWS(input.normal);

   const float3 positionExtrudedWS = extrude_point(positionWS, normalWS,
                                                   light_positionWS.xyz, light_positionWS.w,
                                                   extrusion_offset);

   return mul(float4(positionExtrudedWS, 1.0), projection_matrix);
}

float4 extend_point_hardskin_facenormal_vs(Vs_extend_hardskin_input input) : SV_Position
{
   const int4 blend_indices = input.blend_indices * 255.0;
   const float3 positionWS = hard_skinned_positionWS(input.position, blend_indices.x);
   const float3 normalWS = hard_skinned_normalWS(input.normal, (int)input.blend_indices.y);


   const float3 positionExtrudedWS = extrude_point(positionWS, normalWS,
                                                   light_positionWS.xyz, light_positionWS.w,
                                                   extrusion_offset);

   return mul(float4(positionExtrudedWS, 1.0), projection_matrix);
}

float4 extend_point_hardskin_gen_normal_vs(Vs_extend_hardskin_gen_normal_input input) : SV_Position
{
   const int4 blend_indices = input.blend_indices * 255.0;

   const float3 position0WS = hard_skinned_positionWS(input.position0, blend_indices[0]);
   const float3 position1WS = hard_skinned_positionWS(input.position1, blend_indices[1]);
   const float3 position2WS = hard_skinned_positionWS(input.position2, blend_indices[2]);

   float3 normalWS = cross(position2WS - position1WS, position0WS - position1WS);
   normalWS = normalize(normalWS);

   const float3 positionExtrudedWS = extrude_point(position0WS, normalWS,
                                                   light_positionWS.xyz, light_positionWS.w,
                                                   extrusion_offset);

   return mul(float4(positionExtrudedWS, 1.0), projection_matrix);
}

float4 extend_point_softskin_facenormal_vs(Vs_extend_softskin_facenormal_input input) : SV_Position
{
   const int4 blend_indices = input.blend_indices * 255.0;
   const float3 positionWS = soft_skinned_positionWS(input.position, blend_indices,
                                                     input.blend_weights);

   const int4 face_blend_indices = input.face_blend_indices * 255.0;
   const float3 normalWS = soft_skinned_normalWS(input.face_normal, face_blend_indices,
                                                 input.face_blend_indices);

   const float3 positionExtrudedWS = extrude_point(positionWS, normalWS,
                                                   light_positionWS.xyz, light_positionWS.w,
                                                   extrusion_offset);

   return mul(float4(positionExtrudedWS, 1.0), projection_matrix);
}

float4 extend_point_softskin_gen_normal_vs(Vs_extend_softskin_gen_normal_input input) : SV_Position
{
   const int4 blend_indices[3] = {input.blend_indices0 * 255.0,
                                  input.blend_indices1 * 255.0,
                                  input.blend_indices2 * 255.0};

   const float3 position0WS = soft_skinned_positionWS(input.position0, input.blend_weights0,
                                                      blend_indices[0]);
   const float3 position1WS = soft_skinned_positionWS(input.position1, input.blend_weights1,
                                                      blend_indices[1]);
   const float3 position2WS = soft_skinned_positionWS(input.position2, input.blend_weights2,
                                                      blend_indices[2]);

   float3 normalWS = cross(position2WS - position1WS, position0WS - position1WS);
   normalWS = normalize(normalWS);

   const float3 positionExtrudedWS = extrude_point(position0WS, normalWS,
                                                   light_positionWS.xyz, light_positionWS.w,
                                                   extrusion_offset);

   return mul(float4(positionExtrudedWS, 1.0), projection_matrix);
}

float4 shadow_ps() : SV_Target0
{
   return float4(debug_color, 1.0);
}