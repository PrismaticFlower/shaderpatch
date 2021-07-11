#pragma once

#include <d3d11_4.h>

namespace sp::core {

enum class Game_rendertarget_id : int {};

enum class Game_depthstencil { nearscene, farscene, reflectionscene, none };

enum class Projtex_mode { clamp, wrap };

enum class Projtex_type { tex2d, texcube };

enum class Clear_color { transparent_black, opaque_black };

struct Mapped_texture {
   UINT row_pitch;
   UINT depth_pitch;
   std::byte* data;
};

}
