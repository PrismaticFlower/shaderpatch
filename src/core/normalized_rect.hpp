#pragma once

#include "game_rendertarget.hpp"

namespace sp::core {

struct Normalized_rect {
   double left;
   double top;
   double right;
   double bottom;

   bool operator==(const Normalized_rect&) const noexcept = default;
};

inline auto make_normalized_rect(const RECT& rect, const UINT width,
                                 const UINT height) noexcept -> Normalized_rect
{
   const double width_flt = width;
   const double height_flt = height;

   return {.left = rect.left / width_flt,
           .top = rect.top / height_flt,
           .right = rect.right / width_flt,
           .bottom = rect.bottom / height_flt};
}

inline auto to_box(const Game_rendertarget& rt, const Normalized_rect& rect) noexcept
   -> D3D11_BOX
{
   const double width = rt.width;
   const double height = rt.height;

   return {static_cast<UINT>(rect.left * width),
           static_cast<UINT>(rect.top * height),
           0,
           static_cast<UINT>(rect.right * width),
           static_cast<UINT>(rect.bottom * height),
           1};
}

inline auto to_rect(const Game_rendertarget& rt, const Normalized_rect& rect) noexcept
   -> RECT
{
   const double width = rt.width;
   const double height = rt.height;

   return {static_cast<LONG>(rect.left * width), static_cast<LONG>(rect.top * height),
           static_cast<LONG>(rect.right * width),
           static_cast<LONG>(rect.bottom * height)};
}

}