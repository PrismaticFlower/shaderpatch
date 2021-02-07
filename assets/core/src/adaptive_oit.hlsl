#ifndef ADAPTIVE_OIT_INCLUDED
#define ADAPTIVE_OIT_INCLUDED

#include "constants_list.hlsl"

// clang-format off

namespace aoit
{

RasterizerOrderedTexture2D<uint>  clear_rov : register(u1);
RasterizerOrderedTexture2D<uint4> depth_rov : register(u2);
RasterizerOrderedTexture2D<uint4> color_rov : register(u3);

Texture2D<uint>  clear_srv : register(t0);
Texture2D<uint4> depth_srv : register(t1);
Texture2D<uint4> color_srv : register(t2);

const static uint node_count = 4;
const static float empty_node_depth = asfloat(asuint(3.40282e38) & 0xffffff00);
const static float write_cutoff_alpha = 2.0 / 255;

// Clear surface load/store helpers.
bool clear_surface_load_rov(const uint2 index)
{
   const uint clear_value = clear_rov[index];

   return clear_value != 0;
}

bool clear_surface_load_srv(const uint2 index)
{
   const uint clear_value = clear_srv[index];

   return clear_value != 0;
}

void clear_surface_store(const uint2 index, const bool clear)
{
   clear_rov[index] = clear ? 1 : 0;
}

// Nodes structure definition and load/store functions.
struct Nodes
{
   float depth[node_count];
   float transmittance[node_count];
   float3 color[node_count];
};

uint pack_depth_transmittance(const float depth, const float transmittance)
{
   const uint scaled_transmittance = transmittance * 255.0;
   const uint masked_depth = asuint(depth) & 0xffffff00;

   return scaled_transmittance | masked_depth;
}

void unpack_depth_transmittance(const uint packed_depth_transmittance,
                                out float depth, out float transmittance)
{
   const uint masked_transmittance = packed_depth_transmittance & 0xff;

   transmittance = masked_transmittance * rcp(255.0);
   depth = asfloat(packed_depth_transmittance & 0xffffff00);
}

// Uses R11G11B10 FP Log encoding from MiniEngine.
// https://github.com/microsoft/DirectX-Graphics-Samples/blob/a24eb54454b6d91eb360023e821d91594236bef9/MiniEngine/Core/Shaders/PixelPacking_R11G11B10.hlsli#L42
uint pack_color(const float3 color)
{
   const float3 flat_mantissa = asfloat(asuint(color) & 0x7fffff | 0x3f800000);
   const float3 curved_mantissa = min(log2(flat_mantissa) + 1.0, asfloat(0x3fffffff));
   const float3 log_color = 
      asfloat(asuint(color) & 0xff800000 | asuint(curved_mantissa) & 0x7fffff);

   const uint r = ((f32tof16(log_color.r) + 8) >> 4) & 0x000007ff;
   const uint g = ((f32tof16(log_color.g) + 8) << 7) & 0x003ff800;
   const uint b = ((f32tof16(log_color.b) + 16) << 17) & 0xffc00000;

   return r | g | b;
}

float3 unpack_color(const uint packed_color)
{
   const uint fp16_mask = 0x7ff0;
   const uint3 fp16_color = 
      uint3(packed_color << 4, packed_color >> 7, packed_color >> 17) & fp16_mask;
   const float3 log_color = f16tof32(fp16_color);

   const float3 curved_mantissa = asfloat(asuint(log_color) & 0x7fffff | 0x3f800000);
   const float3 flat_mantissa = exp2(curved_mantissa - 1.0);

   return asfloat(asuint(log_color) & 0xff800000 | asuint(flat_mantissa) & 0x7fffff);
}

Nodes load_nodes(const uint4 depth_data, const uint4 color_data)
{
   Nodes nodes;

   [unroll] 
   for (uint i = 0; i < node_count; ++i) {
      unpack_depth_transmittance(depth_data[i], nodes.depth[i], nodes.transmittance[i]);
      nodes.color[i] = unpack_color(color_data[i]);
   }

   return nodes;
}

Nodes load_nodes_rov(const uint2 index)
{
   const uint4 depth_data = depth_rov[index]; 
   const uint4 color_data = color_rov[index];

   return load_nodes(depth_data, color_data);
}

Nodes load_nodes_srv(const uint2 index)
{
   const uint4 depth_data = depth_srv[index];
   const uint4 color_data = color_srv[index];

   return load_nodes(depth_data, color_data);
}

void store_nodes(const uint2 index, const Nodes nodes)
{
   uint4 depth_data;
   uint4 color_data;

   [unroll]
   for (uint i = 0; i < node_count; ++i)
   {
      depth_data[i] =
         pack_depth_transmittance(nodes.depth[i], nodes.transmittance[i]);
      color_data[i] = pack_color(nodes.color[i]);
   }

   color_rov[index] = color_data;
   depth_rov[index] = depth_data;
}

Nodes create_nodes(const float3 color, const float depth, const float transmittance)
{
   Nodes nodes;

   nodes.depth[0] = depth;
   nodes.transmittance[0] = transmittance;
   nodes.color[0] = color;

   [unroll]
   for (uint i = 1; i < node_count; ++i) {
      nodes.depth[i] = empty_node_depth;
      nodes.transmittance[i] = transmittance;
      nodes.color[i] = float3(0.0, 0.0, 0.0);
   }

   return nodes;
}

void insert_fragment(const float3 frag_color, const float frag_depth,
                     const float frag_transmittance, inout Nodes nodes)
{
   float depth[node_count + 1];
   float transmittance[node_count + 1];
   float3 color[node_count + 1];

   int i; // Silly HLSL for loop variable lifetime rules.
   const int snode_count = (int)node_count;

   [unroll]
   for (i = 0; i < snode_count; ++i) {
      depth[i] = nodes.depth[i];
      transmittance[i] = nodes.transmittance[i];
      color[i] = nodes.color[i];
   }

   int index = 0; 
   float previous_transmittance = 1.0;

   [unroll]
   for (i = 0; i < snode_count; ++i) {
      if (!(frag_depth > depth[i])) continue;

      index += 1;
      previous_transmittance = transmittance[i];
   }

   [unroll]
   for (i = snode_count - 1; i >= 0; --i) {
      if (!(index <= i)) continue;

      depth[i + 1] = depth[i];
      transmittance[i + 1] = transmittance[i] * frag_transmittance;
      color[i + 1] = color[i];
   }

   const float new_node_transmittance = frag_transmittance * previous_transmittance;

   [unroll]
   for (i = 0; i <= snode_count; ++i) {
      if (i != index) continue;

      depth[i] = frag_depth;
      transmittance[i] = new_node_transmittance;
      color[i] = frag_color;
   }

   [flatten]
   if (depth[node_count] != empty_node_depth) {
      const float3 removed_color = color[node_count];
      const float3 scaled_color = removed_color * transmittance[node_count - 1] * rcp(transmittance[node_count - 2]);

      color[node_count - 1] += scaled_color;
      transmittance[node_count - 1] = transmittance[node_count];
   }

   [unroll]
   for (i = 0; i < snode_count; ++i) {
      nodes.depth[i] = depth[i];
      nodes.transmittance[i] = transmittance[i];
      nodes.color[i] = color[i];
   }
}

// Main interface functions.

void write_pixel(const uint2 index, const float depth, const float4 color)
{
   const float clamped_alpha = saturate(color.a);
   const float clamped_depth = saturate(depth);

   const float3 premultiplied_color = color.rgb * clamped_alpha;
   const float transmittance = additive_blending ? 1.0 : 1.0 - clamped_alpha;

   [branch]
   if (clamped_alpha <= write_cutoff_alpha) {
      return;
   }

   const bool clear = clear_surface_load_rov(index);

   [branch] 
   if (clear)
   {
      Nodes nodes = create_nodes(premultiplied_color, clamped_depth, transmittance);

      store_nodes(index, nodes);
   }
   else {
      Nodes nodes = load_nodes_rov(index);

      insert_fragment(premultiplied_color, clamped_depth, transmittance, nodes);

      store_nodes(index, nodes);
   }

   clear_surface_store(index, false);
}

float4 resolve_pixel(const uint2 index)
{
   const bool clear = clear_surface_load_srv(index);

   [branch] 
   if (clear) {
      discard;
      return float4(0.0, 0.0, 0.0, 1.0);
   }

   const Nodes nodes = load_nodes_srv(index);

   float3 color = float3(0.0, 0.0, 0.0);
   float transmittance = 1.0;

   [unroll]
   for (uint i = 0; i < node_count; ++i) {
      color += (nodes.color[i] * transmittance);
      transmittance = nodes.transmittance[i];
   }

   return float4(color, transmittance);
}

}

#endif