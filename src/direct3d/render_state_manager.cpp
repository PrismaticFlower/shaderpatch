
#include "render_state_manager.hpp"
#include "helpers.hpp"
#include "utility.hpp"

#include <algorithm>

namespace sp::d3d9 {

void Render_state_manager::set(const D3DRENDERSTATETYPE state, const DWORD value) noexcept
{
   switch (state) {
      // Rasterizer State
   case D3DRS_FILLMODE:
      _current_rasterizer_state.fill_mode = static_cast<D3DFILLMODE>(value);
      _rasterizer_state_dirty = true;
      break;
   case D3DRS_CULLMODE:
      _current_rasterizer_state.cull_mode = static_cast<D3DCULL>(value);
      _rasterizer_state_dirty = true;
      break;

      // Depth/Stencil State
   case D3DRS_ZENABLE:
      _current_depthstencil_state.depth_enable = (value == D3DZB_TRUE);
      _depthstencil_state_dirty = true;
      break;
   case D3DRS_ZWRITEENABLE:
      _current_depthstencil_state.depth_write_enable = value;
      _depthstencil_state_dirty = true;
      break;
   case D3DRS_ZFUNC:
      _current_depthstencil_state.depth_func = static_cast<D3DCMPFUNC>(value);
      _depthstencil_state_dirty = true;
      break;
   case D3DRS_STENCILENABLE:
      _current_depthstencil_state.stencil_enabled = value;
      _depthstencil_state_dirty = true;
      break;
   case D3DRS_STENCILFAIL:
      _current_depthstencil_state.stencil_fail_op = static_cast<D3DSTENCILOP>(value);
      _depthstencil_state_dirty = true;
      break;
   case D3DRS_STENCILZFAIL:
      _current_depthstencil_state.stencil_depth_fail_op =
         static_cast<D3DSTENCILOP>(value);
      _depthstencil_state_dirty = true;
      break;
   case D3DRS_STENCILPASS:
      _current_depthstencil_state.stencil_pass_op = static_cast<D3DSTENCILOP>(value);
      _depthstencil_state_dirty = true;
      break;
   case D3DRS_STENCILFUNC:
      _current_depthstencil_state.stencil_func = static_cast<D3DCMPFUNC>(value);
      _depthstencil_state_dirty = true;
      break;
   case D3DRS_STENCILREF:
      _current_depthstencil_state.stencil_ref = static_cast<std::uint8_t>(value);
      _depthstencil_state_dirty = true;
      break;
   case D3DRS_STENCILMASK:
      _current_depthstencil_state.stencil_read_mask = static_cast<std::uint8_t>(value);
      _depthstencil_state_dirty = true;
      break;
   case D3DRS_STENCILWRITEMASK:
      _current_depthstencil_state.stencil_write_mask =
         static_cast<std::uint8_t>(value);
      _depthstencil_state_dirty = true;
      break;
   case D3DRS_TWOSIDEDSTENCILMODE:
      _current_depthstencil_state.stencil_doublesided_enabled = value;
      _depthstencil_state_dirty = true;
      break;
   case D3DRS_CCW_STENCILFAIL:
      _current_depthstencil_state.stencil_ccw_fail_op =
         static_cast<D3DSTENCILOP>(value);
      _depthstencil_state_dirty = true;
      break;
   case D3DRS_CCW_STENCILZFAIL:
      _current_depthstencil_state.stencil_ccw_depth_fail_op =
         static_cast<D3DSTENCILOP>(value);
      _depthstencil_state_dirty = true;
      break;
   case D3DRS_CCW_STENCILPASS:
      _current_depthstencil_state.stencil_ccw_pass_op =
         static_cast<D3DSTENCILOP>(value);
      _depthstencil_state_dirty = true;
      break;
   case D3DRS_CCW_STENCILFUNC:
      _current_depthstencil_state.stencil_ccw_func = static_cast<D3DCMPFUNC>(value);
      _depthstencil_state_dirty = true;
      break;

      // Blend State
   case D3DRS_SRCBLEND:
      _current_blend_state.src_blend = static_cast<D3DBLEND>(value);
      _blend_state_dirty = true;
      break;
   case D3DRS_DESTBLEND:
      _current_blend_state.dest_blend = static_cast<D3DBLEND>(value);
      _blend_state_dirty = true;
      break;
   case D3DRS_BLENDOP:
      _current_blend_state.blendop = static_cast<D3DBLENDOP>(value);
      _blend_state_dirty = true;
      break;
   case D3DRS_ALPHABLENDENABLE:
      _current_blend_state.blend_enable = value;
      _blend_state_dirty = true;
      break;
   case D3DRS_COLORWRITEENABLE:
      _current_blend_state.writemask = static_cast<D3DBLENDOP>(value);
      _blend_state_dirty = true;
      break;

      // Fog State
   case D3DRS_FOGENABLE:
      _fog_state.enable = value;
      _fog_state_dirty = true;
      break;
   case D3DRS_FOGCOLOR:
      _fog_state.color = value;
      _fog_state_dirty = true;
      break;

      // Texture Factor
   case D3DRS_TEXTUREFACTOR:
      _texture_factor = value;
      break;
   }
}

void Render_state_manager::reset() noexcept
{
   _blend_state_dirty = true;
   _depthstencil_state_dirty = true;
   _rasterizer_state_dirty = true;
   _fog_state_dirty = true;

   _current_blend_state = {};
   _current_depthstencil_state = {};
   _current_rasterizer_state = {};
   _fog_state = {};
   _texture_factor = 0xffffffff;
}

void Render_state_manager::update_dirty(core::Shader_patch& shader_patch) noexcept
{
   if (_blend_state_dirty) update_blend_state(shader_patch);
   if (_depthstencil_state_dirty) update_depthstencil_state(shader_patch);
   if (_rasterizer_state_dirty) update_rasterizer_state(shader_patch);
   if (_fog_state_dirty) update_fog_state(shader_patch);
}

auto Render_state_manager::texture_factor() const noexcept -> DWORD
{
   return _texture_factor;
}

void Render_state_manager::update_blend_state(core::Shader_patch& shader_patch) noexcept
{
   const bool additive_blending = _current_blend_state.dest_blend == D3DBLEND_ONE;

   if (const auto blend_state =
          std::find_if(_blend_states.cbegin(), _blend_states.cend(),
                       [current_blend_state{_current_blend_state}](const auto& pair) {
                          return (current_blend_state == pair.first);
                       });
       blend_state != _blend_states.cend()) {
      shader_patch.set_blend_state_async(*blend_state->second, additive_blending);

      return;
   }

   ID3D11BlendState1& blend_state =
      *_blend_states
          .emplace_back(_current_blend_state, create_current_blend_state(shader_patch))
          .second;

   shader_patch.set_blend_state_async(blend_state, additive_blending);
}

void Render_state_manager::update_depthstencil_state(core::Shader_patch& shader_patch) noexcept
{
   const bool depth_readonly = _current_depthstencil_state.depth_write_enable == 0;
   const bool stencil_readonly =
      (_current_depthstencil_state.stencil_write_mask == 0) |
      ((_current_depthstencil_state.stencil_enabled == 0) &
       (_current_depthstencil_state.stencil_doublesided_enabled == 0));
   const bool readonly_depthstencil = depth_readonly & stencil_readonly;

   if (const auto depthstencil_state =
          std::find_if(_depthstencil_states.cbegin(), _depthstencil_states.cend(),
                       [current_depthstencil_state{_current_depthstencil_state}](
                          const auto& pair) {
                          return (current_depthstencil_state == pair.first);
                       });
       depthstencil_state != _depthstencil_states.cend()) {
      shader_patch.set_depthstencil_state_async(*depthstencil_state->second,
                                                _current_depthstencil_state.stencil_ref,
                                                readonly_depthstencil);

      return;
   }

   ID3D11DepthStencilState& depthstencil_state =
      *_depthstencil_states
          .emplace_back(_current_depthstencil_state,
                        create_current_depthstencil_state(shader_patch))
          .second;

   shader_patch.set_depthstencil_state_async(depthstencil_state,
                                             _current_depthstencil_state.stencil_ref,
                                             readonly_depthstencil);
}

void Render_state_manager::update_rasterizer_state(core::Shader_patch& shader_patch) noexcept
{
   if (const auto rasterizer_state =
          std::find_if(_rasterizer_states.cbegin(), _rasterizer_states.cend(),
                       [current_rasterizer_state{_current_rasterizer_state}](
                          const auto& pair) {
                          return (current_rasterizer_state == pair.first);
                       });
       rasterizer_state != _rasterizer_states.cend()) {
      shader_patch.set_rasterizer_state_async(*rasterizer_state->second);

      return;
   }

   ID3D11RasterizerState& rasterizer_state =
      *_rasterizer_states
          .emplace_back(_current_rasterizer_state,
                        create_current_rasterizer_state(shader_patch))
          .second;

   shader_patch.set_rasterizer_state_async(rasterizer_state);
}

void Render_state_manager::update_fog_state(core::Shader_patch& shader_patch) noexcept
{
   shader_patch.set_fog_state_async(_fog_state.enable,
                                    unpack_d3dcolor(_fog_state.color));
}

auto Render_state_manager::create_current_blend_state(core::Shader_patch& shader_patch) const noexcept
   -> Com_ptr<ID3D11BlendState1>
{
   D3D11_RENDER_TARGET_BLEND_DESC1 desc{true, false};

   const auto map_blend_value = [](const unsigned int d3d9_blend) noexcept {
      switch (d3d9_blend) {
      case D3DBLEND_ZERO:
         return D3D11_BLEND_ZERO;
      case D3DBLEND_ONE:
         return D3D11_BLEND_ONE;
      case D3DBLEND_SRCCOLOR:
         return D3D11_BLEND_SRC_COLOR;
      case D3DBLEND_INVSRCCOLOR:
         return D3D11_BLEND_INV_SRC_COLOR;
      case D3DBLEND_SRCALPHA:
         return D3D11_BLEND_SRC_ALPHA;
      case D3DBLEND_INVSRCALPHA:
         return D3D11_BLEND_INV_SRC_ALPHA;
      case D3DBLEND_DESTALPHA:
         return D3D11_BLEND_DEST_ALPHA;
      case D3DBLEND_INVDESTALPHA:
         return D3D11_BLEND_INV_DEST_ALPHA;
      case D3DBLEND_DESTCOLOR:
         return D3D11_BLEND_DEST_COLOR;
      case D3DBLEND_INVDESTCOLOR:
         return D3D11_BLEND_INV_DEST_COLOR;
      case D3DBLEND_SRCALPHASAT:
         return D3D11_BLEND_SRC_ALPHA_SAT;
      case D3DBLEND_BLENDFACTOR:
         return D3D11_BLEND_BLEND_FACTOR;
      case D3DBLEND_INVBLENDFACTOR:
         return D3D11_BLEND_INV_BLEND_FACTOR;
      default:
         return D3D11_BLEND_ZERO;
      }
   };

   const auto map_alpha_blend_value = [](const unsigned int d3d9_blend) noexcept {
      switch (d3d9_blend) {
      case D3DBLEND_ZERO:
         return D3D11_BLEND_ZERO;
      case D3DBLEND_ONE:
         return D3D11_BLEND_ONE;
      case D3DBLEND_SRCCOLOR:
      case D3DBLEND_SRCALPHA:
         return D3D11_BLEND_SRC_ALPHA;
      case D3DBLEND_INVSRCCOLOR:
      case D3DBLEND_INVSRCALPHA:
         return D3D11_BLEND_INV_SRC_ALPHA;
      case D3DBLEND_DESTALPHA:
      case D3DBLEND_DESTCOLOR:
         return D3D11_BLEND_DEST_ALPHA;
      case D3DBLEND_INVDESTALPHA:
      case D3DBLEND_INVDESTCOLOR:
         return D3D11_BLEND_INV_DEST_ALPHA;
      case D3DBLEND_SRCALPHASAT:
         return D3D11_BLEND_SRC_ALPHA_SAT;
      case D3DBLEND_BLENDFACTOR:
         return D3D11_BLEND_BLEND_FACTOR;
      case D3DBLEND_INVBLENDFACTOR:
         return D3D11_BLEND_INV_BLEND_FACTOR;
      default:
         return D3D11_BLEND_ZERO;
      }
   };

   const auto map_blendop_value = [](const unsigned int d3d9_blendop) noexcept {
      switch (d3d9_blendop) {
      case D3DBLENDOP_ADD:
         return D3D11_BLEND_OP_ADD;
      case D3DBLENDOP_SUBTRACT:
         return D3D11_BLEND_OP_SUBTRACT;
      case D3DBLENDOP_REVSUBTRACT:
         return D3D11_BLEND_OP_REV_SUBTRACT;
      case D3DBLENDOP_MIN:
         return D3D11_BLEND_OP_MIN;
      case D3DBLENDOP_MAX:
         return D3D11_BLEND_OP_MAX;
      default:
         return D3D11_BLEND_OP_ADD;
      }
   };

   desc.BlendEnable = _current_blend_state.blend_enable;

   if (_current_blend_state.blend_enable) {
      desc.SrcBlend = map_blend_value(_current_blend_state.src_blend);
      desc.DestBlend = map_blend_value(_current_blend_state.dest_blend);
      desc.BlendOp = map_blendop_value(_current_blend_state.blendop);
      desc.SrcBlendAlpha = map_alpha_blend_value(_current_blend_state.src_blend);
      desc.DestBlendAlpha = map_alpha_blend_value(_current_blend_state.dest_blend);
      desc.BlendOpAlpha = desc.BlendOp;
   }
   else {
      desc.SrcBlend = D3D11_BLEND_ONE;
      desc.DestBlend = D3D11_BLEND_ZERO;
      desc.BlendOp = D3D11_BLEND_OP_ADD;
      desc.SrcBlendAlpha = D3D11_BLEND_ONE;
      desc.DestBlendAlpha = D3D11_BLEND_ZERO;
      desc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
   }

   desc.RenderTargetWriteMask = _current_blend_state.writemask;
   desc.LogicOp = D3D11_LOGIC_OP_CLEAR;

   return shader_patch.create_blend_state({false, false, desc});
}

auto Render_state_manager::create_current_depthstencil_state(
   core::Shader_patch& shader_patch) const noexcept -> Com_ptr<ID3D11DepthStencilState>
{
   D3D11_DEPTH_STENCIL_DESC desc{};

   const auto map_cmp_func_value = [](const unsigned int func) noexcept -> D3D11_COMPARISON_FUNC {
      switch (func) {
      case D3DCMP_NEVER:
         return D3D11_COMPARISON_NEVER;
      case D3DCMP_LESS:
         return D3D11_COMPARISON_LESS;
      case D3DCMP_EQUAL:
         return D3D11_COMPARISON_EQUAL;
      case D3DCMP_LESSEQUAL:
         return D3D11_COMPARISON_LESS_EQUAL;
      case D3DCMP_GREATER:
         return D3D11_COMPARISON_GREATER;
      case D3DCMP_NOTEQUAL:
         return D3D11_COMPARISON_NOT_EQUAL;
      case D3DCMP_GREATEREQUAL:
         return D3D11_COMPARISON_GREATER_EQUAL;
      case D3DCMP_ALWAYS:
         return D3D11_COMPARISON_ALWAYS;
      default:
         return D3D11_COMPARISON_ALWAYS;
      }
   };

   const auto map_stencil_op_value = [](const unsigned int op) noexcept -> D3D11_STENCIL_OP {
      switch (op) {
      case D3DSTENCILOP_KEEP:
         return D3D11_STENCIL_OP_KEEP;
      case D3DSTENCILOP_ZERO:
         return D3D11_STENCIL_OP_ZERO;
      case D3DSTENCILOP_REPLACE:
         return D3D11_STENCIL_OP_REPLACE;
      case D3DSTENCILOP_INCRSAT:
         return D3D11_STENCIL_OP_INCR_SAT;
      case D3DSTENCILOP_DECRSAT:
         return D3D11_STENCIL_OP_DECR_SAT;
      case D3DSTENCILOP_INVERT:
         return D3D11_STENCIL_OP_INVERT;
      case D3DSTENCILOP_INCR:
         return D3D11_STENCIL_OP_INCR;
      case D3DSTENCILOP_DECR:
         return D3D11_STENCIL_OP_DECR;
      default:
         return D3D11_STENCIL_OP_KEEP;
      }
   };

   desc.DepthEnable = _current_depthstencil_state.depth_enable;
   desc.DepthWriteMask = _current_depthstencil_state.depth_write_enable
                            ? D3D11_DEPTH_WRITE_MASK_ALL
                            : D3D11_DEPTH_WRITE_MASK_ZERO;
   desc.DepthFunc = map_cmp_func_value(_current_depthstencil_state.depth_func);

   desc.StencilEnable = _current_depthstencil_state.stencil_enabled;
   desc.StencilReadMask = _current_depthstencil_state.stencil_read_mask;
   desc.StencilWriteMask = _current_depthstencil_state.stencil_write_mask;

   desc.FrontFace.StencilFailOp =
      map_stencil_op_value(_current_depthstencil_state.stencil_fail_op);
   desc.FrontFace.StencilDepthFailOp =
      map_stencil_op_value(_current_depthstencil_state.stencil_depth_fail_op);
   desc.FrontFace.StencilPassOp =
      map_stencil_op_value(_current_depthstencil_state.stencil_pass_op);
   desc.FrontFace.StencilFunc =
      map_cmp_func_value(_current_depthstencil_state.stencil_func);

   if (_current_depthstencil_state.stencil_doublesided_enabled) {
      desc.BackFace.StencilFailOp =
         map_stencil_op_value(_current_depthstencil_state.stencil_ccw_fail_op);
      desc.BackFace.StencilDepthFailOp =
         map_stencil_op_value(_current_depthstencil_state.stencil_ccw_depth_fail_op);
      desc.BackFace.StencilPassOp =
         map_stencil_op_value(_current_depthstencil_state.stencil_ccw_pass_op);
      desc.BackFace.StencilFunc =
         map_cmp_func_value(_current_depthstencil_state.stencil_ccw_func);
   }
   else {
      desc.BackFace = desc.FrontFace;
   }

   return shader_patch.create_depthstencil_state(desc);
}

auto Render_state_manager::create_current_rasterizer_state(
   core::Shader_patch& shader_patch) const noexcept -> Com_ptr<ID3D11RasterizerState>
{
   D3D11_RASTERIZER_DESC desc{};

   const auto map_cull_mode = [](const unsigned int mode) noexcept -> D3D11_CULL_MODE {
      switch (mode) {
      case D3DCULL_NONE:
         return D3D11_CULL_NONE;
      case D3DCULL_CW:
         return D3D11_CULL_FRONT;
      case D3DCULL_CCW:
         return D3D11_CULL_BACK;
      default:
         return D3D11_CULL_NONE;
      }
   };

   desc.FillMode = _current_rasterizer_state.fill_mode == D3DFILL_WIREFRAME
                      ? D3D11_FILL_WIREFRAME
                      : D3D11_FILL_SOLID;
   desc.CullMode = map_cull_mode(_current_rasterizer_state.cull_mode);
   desc.FrontCounterClockwise = false;
   desc.DepthBias = 0;
   desc.DepthBiasClamp = 0.0f;
   desc.SlopeScaledDepthBias = 0.0f;
   desc.DepthClipEnable = true;
   desc.ScissorEnable = false;
   desc.MultisampleEnable = false;
   desc.AntialiasedLineEnable = false;

   return shader_patch.create_rasterizer_state(desc);
}

bool operator==(const Render_state_manager::Blend_state left,
                const Render_state_manager::Blend_state right) noexcept
{
   static_assert(sizeof(Render_state_manager::Blend_state) <= sizeof(int));
   static_assert(std::is_trivially_copyable_v<Render_state_manager::Blend_state>);

   return bit_cast<std::int32_t>(left) == bit_cast<std::int32_t>(right);
}

bool operator==(const Render_state_manager::Depthstencil_state left,
                const Render_state_manager::Depthstencil_state right) noexcept
{
   static_assert(sizeof(Render_state_manager::Blend_state) <= sizeof(int));
   static_assert(std::is_trivially_copyable_v<Render_state_manager::Blend_state>);

   return bit_cast<std::int64_t>(left) == bit_cast<std::int64_t>(right);
}

bool operator==(const Render_state_manager::Rasterizer_state left,
                const Render_state_manager::Rasterizer_state right) noexcept
{
   static_assert(sizeof(Render_state_manager::Rasterizer_state) <= sizeof(int));
   static_assert(std::is_trivially_copyable_v<Render_state_manager::Rasterizer_state>);

   return bit_cast<std::int32_t>(left) == bit_cast<std::int32_t>(right);
}

}
