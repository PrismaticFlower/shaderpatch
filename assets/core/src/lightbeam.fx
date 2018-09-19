#include "constants_list.hlsl"
#include "fog_utilities.hlsl"
#include "generic_vertex_input.hlsl"
#include "vertex_transformer.hlsl"

const static float lightbeam_fog_scale = 0.66;

struct Vs_input
{
   float4 position : POSITION;
   float4 color : COLOR;
};

struct Vs_output
{
   float4 positionPS : SV_Position;
   float4 color : TEXCOORD;
   float fog_eye_distance : DEPTH;
};

Vs_output lightbeam_vs(Vertex_input input)
{
   Vs_output output;

   Transformer transformer = create_transformer(input);

   float3 positionWS = transformer.positionWS();

   output.positionPS = transformer.positionPS();
   output.fog_eye_distance = fog::get_eye_distance(positionWS) * lightbeam_fog_scale;

   Near_scene near_scene = calculate_near_scene_fade(positionWS);
   near_scene.fade = saturate(near_scene.fade * near_scene.fade);

   float4 material_color = get_material_color(input.color());

   output.color.rgb = material_color.rgb * hdr_info.z;
   output.color.a = saturate(material_color.a * near_scene.fade);

   return output;
}

struct Ps_input
{
   float4 color : TEXCOORD;
   float fog_eye_distance : DEPTH;
};

float4 lightbeam_ps(Ps_input input) : SV_Target0
{
   float4 color = input.color;

   color.rgb = fog::apply(color.rgb, input.fog_eye_distance);

   return color;
}