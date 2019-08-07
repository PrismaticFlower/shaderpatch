
#include "terrain_cut.hpp"
#include "terrain_vertex_buffer.hpp"

#include <limits>

namespace sp {

void Terrain_cut::apply(Terrain_triangle_list& tris) const noexcept
{
   if (planes.empty()) return;

   // TODO: Clip triangles here.
}
}
