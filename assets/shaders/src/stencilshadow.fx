
#include "vertex_utilities.hlsl"
#include "transform_utilities.hlsl"

struct Vs_input
{
   float4 position : POSITION;
   float3 normal : NORMAL;
   float4 weights : BLENDWEIGHT;
   uint4 blend_indices : BLENDINDICES;
};

struct Vs_input_generate_normal
{
   float4 positions[3] : POSITION;
   float4 weights[3] : BLENDWEIGHT;
   uint4 blend_indices[3] : BLENDINDICES;
};

float4 preextended_vs(float4 position : POSITION) : POSITION
{
   return transform::position_project(position);
}

float4 extrude_directional(float4 world_position, float3 world_normal,
                           float3 light_direction, float2 extrusion_info)
{
   float normal_dot = dot(world_normal.xyz, -light_direction);

   if (normal_dot >= 0.0) normal_dot = 1.0f;
   else normal_dot = 0.0f;

   const float extrusion_length = extrusion_info.x;
   const float extrusion_offset = extrusion_info.y;

   normal_dot = normal_dot * extrusion_length + extrusion_offset;

   return float4(normal_dot * light_direction + world_position.xyz, world_position.w);
}

float4 extend_directional_vs(Vs_input input, 
                             uniform float3 light_direction : register(vs, c[CUSTOM_CONST_MIN]),
                             uniform float2 extrusion_info : register(vs, c[CUSTOM_CONST_MIN + 1])) : POSITION
{
   float4 world_position = transform::position(input.position);
   float3 world_normal = normals_to_world(decompress_normals(input.normal));

   float4 extruded_position = extrude_directional(world_position, world_normal,
                                                  light_direction, extrusion_info);

   return position_project(extruded_position);
}

float4 extend_directional_hardskin_facenormal_vs(Vs_input input,
                                                 uniform float3 light_direction : register(vs, c[CUSTOM_CONST_MIN]),
                                                 uniform float2 extrusion_info : register(vs, c[CUSTOM_CONST_MIN + 1])) : POSITION
{
   float4 obj_position = transform::hard_skinned::position(input.position, 
                                                           input.blend_indices);
   float3 obj_normal = transform::hard_skinned::normals(input.normal,
                                                        input.blend_indices);

   float4 world_position = position_to_world(obj_position);
   float3 world_normal = normals_to_world(obj_normal);

   float4 extruded_position = extrude_directional(world_position, world_normal,
                                                  light_direction, extrusion_info);

   return position_project(extruded_position);
}

float4 extend_directional_hardskin_gen_normal_vs(Vs_input_generate_normal input,
                                                 uniform float3 light_direction : register(vs, c[CUSTOM_CONST_MIN]),
                                                 uniform float2 extrusion_info : register(vs, c[CUSTOM_CONST_MIN + 1])) : POSITION
{
   float4 obj_positions[3];

   for (uint i = 0; i < 3; ++i) {
      obj_positions[i] = transform::hard_skinned::position(input.positions[i],
                                                           input.blend_indices[0][i]);
   }

   float3 obj_normal = cross(obj_positions[2].xyz - obj_positions[1].xyz,
                             obj_positions[0].xyz - obj_positions[1].xyz);

   float4 world_position = position_to_world(obj_positions[0]);
   float3 world_normal = normals_to_world(obj_normal);

   float4 extruded_position = extrude_directional(world_position, world_normal,
                                                  light_direction, extrusion_info);

   return position_project(extruded_position);
}

float4 extend_directional_softskin_facenormal_vs(Vs_input input,
                                                 uniform float3 light_direction : register(vs, c[CUSTOM_CONST_MIN]),
                                                 uniform float2 extrusion_info : register(vs, c[CUSTOM_CONST_MIN + 1])) : POSITION
{
   float4 obj_position = transform::soft_skinned::position(input.position, 
                                                           input.blend_indices,
                                                           input.weights);
   float3 obj_normal = transform::soft_skinned::normals(input.normal,
                                                        input.blend_indices,
                                                        input.weights);

   float4 world_position = position_to_world(obj_position);
   float3 world_normal = normals_to_world(obj_normal);

   float4 extruded_position = extrude_directional(world_position, world_normal,
                                                  light_direction, extrusion_info);

   return position_project(extruded_position);
}

float4 extend_directional_softskin_gen_normal_vs(Vs_input_generate_normal input,
                                                 uniform float3 light_direction : register(vs, c[CUSTOM_CONST_MIN]),
                                                 uniform float2 extrusion_info : register(vs, c[CUSTOM_CONST_MIN + 1])) : POSITION
{
   float4 obj_positions[3];

   for (uint i = 0; i < 3; ++i) {
      obj_positions[i] = transform::soft_skinned::position(input.positions[i],
                                                           input.blend_indices[i],
                                                           input.weights[i]);
   }

   float3 obj_normal = cross(obj_positions[2].xyz - obj_positions[1].xyz,
                             obj_positions[0].xyz - obj_positions[1].xyz);

   float4 world_position = position_to_world(obj_positions[0]);
   float3 world_normal = normals_to_world(obj_normal);

   float4 extruded_position = extrude_directional(world_position, world_normal,
                                                  light_direction, extrusion_info);

   return position_project(extruded_position);
}

float4 extrude_point(float4 world_position, float3 world_normal, float3 light_position,
                     float light_radius, float extrusion_offset)
{
   float3 light_direction = world_position.xyz - light_position.xyz;

   float dir_dot = dot(light_direction, light_direction);
   float dir_dot_sqr = rsqrt(dir_dot);

   light_direction *= dir_dot_sqr;
   dir_dot *= dir_dot_sqr;

   float extrude_amount = max(light_radius - dir_dot, 0.0);

   float normal_dot = dot(world_normal.xyz, -light_direction);

   normal_dot = (normal_dot >= 0.0) ? 1.0 : 0.0;

   normal_dot = normal_dot * extrude_amount + extrusion_offset;

   return float4(normal_dot * light_direction + world_position.xyz, world_position.w);
}

float4 extend_point_vs(Vs_input input, 
                       uniform float4 light_position : register(vs, c[CUSTOM_CONST_MIN]),
                       uniform float extrusion_offset : register(vs, c[CUSTOM_CONST_MIN + 1])) : POSITION
{
   float4 world_position = transform::position(input.position);
   float3 world_normal = normals_to_world(decompress_normals(input.normal));

   float4 extruded_position = extrude_point(world_position, world_normal, 
                                            light_position.xyz, light_position.w, 
                                            extrusion_offset);

   return position_project(extruded_position);
}

float4 extend_point_hardskin_facenormal_vs(Vs_input input, 
                                           uniform float4 light_position : register(vs, c[CUSTOM_CONST_MIN]),
                                           uniform float extrusion_offset : register(vs, c[CUSTOM_CONST_MIN + 1])) : POSITION
{
   float4 obj_position = transform::hard_skinned::position(input.position, 
                                                           input.blend_indices);
   float3 obj_normal = transform::hard_skinned::normals(input.normal,
                                                        input.blend_indices);

   float4 world_position = position_to_world(obj_position);
   float3 world_normal = normals_to_world(obj_normal);
   
   float4 extruded_position = extrude_point(world_position, world_normal, 
                                            light_position.xyz, light_position.w, 
                                            extrusion_offset);

   return position_project(extruded_position);
}

float4 extend_point_hardskin_gen_normal_vs(Vs_input_generate_normal input, 
                                           uniform float4 light_position : register(vs, c[CUSTOM_CONST_MIN]),
                                           uniform float extrusion_offset : register(vs, c[CUSTOM_CONST_MIN + 1])) : POSITION
{
   float4 obj_positions[3];

   for (uint i = 0; i < 3; ++i) {
      obj_positions[i] = transform::hard_skinned::position(input.positions[i],
                                                           input.blend_indices[0][i]);
   }

   float3 obj_normal = cross(obj_positions[2].xyz - obj_positions[1].xyz,
                             obj_positions[0].xyz - obj_positions[1].xyz);

   float4 world_position = position_to_world(obj_positions[0]);
   float3 world_normal = normals_to_world(obj_normal);
   
   float4 extruded_position = extrude_point(world_position, world_normal, 
                                            light_position.xyz, light_position.w, 
                                            extrusion_offset);

   return position_project(extruded_position);
}

float4 extend_point_softskin_facenormal_vs(Vs_input input,
                                           uniform float4 light_position : register(vs, c[CUSTOM_CONST_MIN]),
                                           uniform float extrusion_offset : register(vs, c[CUSTOM_CONST_MIN + 1])) : POSITION
{
   float4 obj_position = transform::soft_skinned::position(input.position, 
                                                           input.blend_indices,
                                                           input.weights);
   float3 obj_normal = transform::soft_skinned::normals(input.normal,
                                                        input.blend_indices,
                                                        input.weights);

   float4 world_position = position_to_world(obj_position);
   float3 world_normal = normals_to_world(obj_normal);
   
   float4 extruded_position = extrude_point(world_position, world_normal, 
                                            light_position.xyz, light_position.w, 
                                            extrusion_offset);

   return position_project(extruded_position);
}

float4 extend_point_softskin_gen_normal_vs(Vs_input_generate_normal input,
                                           uniform float4 light_position : register(vs, c[CUSTOM_CONST_MIN]),
                                           uniform float extrusion_offset : register(vs, c[CUSTOM_CONST_MIN + 1])) : POSITION
{
   float4 obj_positions[3];

   for (uint i = 0; i < 3; ++i) {
      obj_positions[i] = transform::soft_skinned::position(input.positions[i],
                                                           input.blend_indices[i],
                                                           input.weights[i]);
   }

   float3 obj_normal = cross(obj_positions[2].xyz - obj_positions[1].xyz,
                             obj_positions[0].xyz - obj_positions[1].xyz);

   float4 world_position = position_to_world(obj_positions[0]);
   float3 world_normal = normals_to_world(obj_normal);
   
   float4 extruded_position = extrude_point(world_position, world_normal, 
                                            light_position.xyz, light_position.w, 
                                            extrusion_offset);

   return position_project(extruded_position);
}

float4 shadow_ps() : COLOR
{
   return float4(0.0, 0.0, 0.0, 1.0);
}