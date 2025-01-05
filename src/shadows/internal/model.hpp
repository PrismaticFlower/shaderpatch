#pragma once

#include "com_ptr.hpp"

#include <array>
#include <vector>

#include <d3d11.h>

namespace sp::shadows {

struct Model_merge_segment {
   D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;

   std::vector<std::array<std::int16_t, 3>> vertices;
   std::vector<std::uint16_t> indices;
};

struct Model_merge_segment_hardedged {
   D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;

   std::vector<std::array<std::int16_t, 5>> vertices;
   std::vector<std::uint16_t> indices;

   std::uint32_t texture_name_hash;
};

struct Model_segment {
   D3D11_PRIMITIVE_TOPOLOGY topology;
   UINT index_count;
   UINT start_index;
   INT base_vertex;
};

struct Model_segment_hardedged {
   D3D11_PRIMITIVE_TOPOLOGY topology;
   UINT index_count;
   UINT start_index;
   INT base_vertex;

   Com_ptr<ID3D11Texture2D> texture;
};

struct Model {

   // TODO: Avoid allocations for simple meshes that fit in a single segment.
   std::vector<Model_segment> opaque_segments;
   std::vector<Model_segment> doublesided_segments;
   std::vector<Model_segment_hardedged> hardedged_segments;
   std::vector<Model_segment_hardedged> hardedged_doublesided_segments;
};

}