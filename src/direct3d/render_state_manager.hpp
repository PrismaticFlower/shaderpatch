#pragma once

#include "../core/shader_patch.hpp"

#include <cstdint>
#include <utility>
#include <vector>

#include <d3d9.h>

#pragma warning(push)
#pragma warning(disable : 4324)

namespace sp::d3d9 {

class Render_state_manager {
public:
   void set(const D3DRENDERSTATETYPE state, const DWORD value) noexcept;

   void reset() noexcept;

   void update_dirty(core::Shader_patch& shader_patch) noexcept;

   auto texture_factor() const noexcept -> DWORD;

private:
   void update_blend_state(core::Shader_patch& shader_patch) noexcept;

   void update_depthstencil_state(core::Shader_patch& shader_patch) noexcept;

   void update_rasterizer_state(core::Shader_patch& shader_patch) noexcept;

   void update_fog_state(core::Shader_patch& shader_patch) noexcept;

   auto create_current_blend_state(core::Shader_patch& shader_patch) const
      noexcept -> Com_ptr<ID3D11BlendState1>;

   auto create_current_depthstencil_state(core::Shader_patch& shader_patch) const
      noexcept -> Com_ptr<ID3D11DepthStencilState>;

   auto create_current_rasterizer_state(core::Shader_patch& shader_patch) const
      noexcept -> Com_ptr<ID3D11RasterizerState>;

   struct alignas(std::int32_t) Blend_state {
      unsigned int src_blend : 4;
      unsigned int dest_blend : 4;
      unsigned int blendop : 2;
      unsigned int writemask : 4;
      unsigned int blend_enable : 1;

      Blend_state() noexcept
         : src_blend{D3DBLEND_ONE}, dest_blend{D3DBLEND_ZERO}, blendop{D3DBLENDOP_ADD}, writemask{0b1111u}, blend_enable{false}
      {
      }
   };

   static_assert(sizeof(Blend_state) == sizeof(std::int32_t));

   friend bool operator==(const Blend_state left, const Blend_state right) noexcept;

#pragma pack(push, 1)
   struct alignas(std::int64_t) Depthstencil_state {
      unsigned int depth_enable : 1;
      unsigned int depth_write_enable : 1;
      unsigned int stencil_enabled : 1;
      unsigned int stencil_doublesided_enabled : 1;
      unsigned int depth_func : 4;
      unsigned int stencil_func : 4;
      unsigned int stencil_ccw_func : 4;
      unsigned int stencil_pass_op : 4;
      unsigned int stencil_fail_op : 4;
      unsigned int stencil_depth_fail_op : 4;
      unsigned int stencil_ccw_pass_op : 4;
      unsigned int stencil_ccw_fail_op : 4;
      unsigned int stencil_ccw_depth_fail_op : 4;
      unsigned int stencil_read_mask : 8;
      unsigned int stencil_write_mask : 8;
      unsigned int stencil_ref : 8;

      Depthstencil_state() noexcept
         : depth_enable{true},
           depth_write_enable{true},
           stencil_enabled{false},
           stencil_doublesided_enabled{false},
           depth_func{D3DCMP_LESSEQUAL},
           stencil_func{D3DCMP_ALWAYS},
           stencil_ccw_func{D3DCMP_ALWAYS},
           stencil_pass_op{D3DSTENCILOP_KEEP},
           stencil_fail_op{D3DSTENCILOP_KEEP},
           stencil_depth_fail_op{D3DSTENCILOP_KEEP},
           stencil_ccw_pass_op{D3DSTENCILOP_KEEP},
           stencil_ccw_fail_op{D3DSTENCILOP_KEEP},
           stencil_ccw_depth_fail_op{D3DSTENCILOP_KEEP},
           stencil_read_mask{0xff},
           stencil_write_mask{0xff},
           stencil_ref{0xff}
      {
      }
   };
#pragma pack(pop)

   static_assert(sizeof(Depthstencil_state) == sizeof(std::int64_t));

   friend bool operator==(const Depthstencil_state left,
                          const Depthstencil_state right) noexcept;

   struct alignas(std::int32_t) Rasterizer_state {
      unsigned int fill_mode : 2;
      unsigned int cull_mode : 2;

      Rasterizer_state() noexcept
         : fill_mode{D3DFILL_SOLID}, cull_mode{D3DCULL_CCW}
      {
      }
   };

   static_assert(sizeof(Rasterizer_state) == sizeof(std::int32_t));

   friend bool operator==(const Rasterizer_state left,
                          const Rasterizer_state right) noexcept;

   struct Fog_state {
      D3DCOLOR color = 0x0;
      bool enable = false;
   };

   bool _blend_state_dirty = true;
   bool _depthstencil_state_dirty = true;
   bool _rasterizer_state_dirty = true;
   bool _fog_state_dirty = true;

   Blend_state _current_blend_state;
   std::vector<std::pair<Blend_state, Com_ptr<ID3D11BlendState1>>> _blend_states;

   Depthstencil_state _current_depthstencil_state;
   std::vector<std::pair<Depthstencil_state, Com_ptr<ID3D11DepthStencilState>>> _depthstencil_states;

   Rasterizer_state _current_rasterizer_state;
   std::vector<std::pair<Rasterizer_state, Com_ptr<ID3D11RasterizerState>>> _rasterizer_states;

   Fog_state _fog_state;
   DWORD _texture_factor = 0xffffffff;
};

}

#pragma warning(pop)
