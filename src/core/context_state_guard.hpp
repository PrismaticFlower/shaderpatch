#pragma once

#include "com_ptr.hpp"

#include <array>
#include <type_traits>

#include <d3d11_1.h>

namespace sp::core {

namespace state_flags {
constexpr auto cs_cbuffers = 1ull << 0;
constexpr auto cs_samplers = 1ull << 1;
constexpr auto cs_shader = 1ull << 2;
constexpr auto cs_resources = 1ull << 3;
constexpr auto cs_uavs = 1ull << 4;

constexpr auto ia_ibuffer = 1ull << 5;
constexpr auto ia_layout = 1ull << 6;
constexpr auto ia_topology = 1ull << 7;
constexpr auto ia_vbuffers = 1ull << 8;

constexpr auto vs_cbuffers = 1ull << 9;
constexpr auto vs_samplers = 1ull << 10;
constexpr auto vs_shader = 1ull << 11;
constexpr auto vs_resources = 1ull << 12;

constexpr auto hs_cbuffers = 1ull << 13;
constexpr auto hs_samplers = 1ull << 14;
constexpr auto hs_shader = 1ull << 15;
constexpr auto hs_resources = 1ull << 16;

constexpr auto ds_cbuffers = 1ull << 17;
constexpr auto ds_samplers = 1ull << 18;
constexpr auto ds_shader = 1ull << 19;
constexpr auto ds_resources = 1ull << 20;

constexpr auto gs_cbuffers = 1ull << 21;
constexpr auto gs_samplers = 1ull << 22;
constexpr auto gs_shader = 1ull << 23;
constexpr auto gs_resources = 1ull << 24;

constexpr auto rs_scissors = 1ull << 25;
constexpr auto rs_state = 1ull << 26;
constexpr auto rs_viewports = 1ull << 27;

constexpr auto ps_cbuffers = 1ull << 28;
constexpr auto ps_samplers = 1ull << 29;
constexpr auto ps_shader = 1ull << 30;
constexpr auto ps_resources = 1ull << 31;

constexpr auto om_blend_state = 1ull << 32;
constexpr auto om_ds_state = 1ull << 33;
constexpr auto om_dsv_rtvs = 1ull << 34;

constexpr auto cs_all = cs_cbuffers | cs_samplers | cs_shader | cs_resources | cs_uavs;
constexpr auto ia_all = ia_ibuffer | ia_layout | ia_topology | ia_vbuffers;
constexpr auto vs_all = vs_cbuffers | vs_samplers | vs_shader | vs_resources;
constexpr auto hs_all = hs_cbuffers | hs_samplers | hs_shader | hs_resources;
constexpr auto ds_all = ds_cbuffers | ds_samplers | ds_shader | ds_resources;
constexpr auto gs_all = gs_cbuffers | gs_samplers | gs_shader | gs_resources;
constexpr auto rs_all = rs_scissors | rs_state | rs_viewports;
constexpr auto ps_all = ps_cbuffers | ps_samplers | ps_shader | ps_resources;
constexpr auto om_all = om_blend_state | om_ds_state | om_dsv_rtvs;
constexpr auto all = cs_all | ia_all | vs_all | hs_all | ds_all | gs_all |
                     rs_all | ps_all | om_all;

}

namespace detail {
// Avert your eyes now good soul, and remain innocent.

#define CREATE_SHADER_CBUFFER_TYPE(prefix, func_prefix)                                                      \
   struct prefix##_cbuffers {                                                                                \
      std::array<Com_ptr<ID3D11Buffer>, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT> prefix##_buffers; \
      std::array<UINT, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT> prefix##_first_constants;          \
      std::array<UINT, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT> prefix##_num_constants;            \
                                                                                                             \
      void fill(ID3D11DeviceContext1& dc) noexcept                                                           \
      {                                                                                                      \
         prefix##_buffers = {};                                                                              \
         dc.func_prefix##GetConstantBuffers1(0, prefix##_buffers.size(),                                     \
                                             reinterpret_cast<ID3D11Buffer**>(                               \
                                                prefix##_buffers.data()),                                    \
                                             prefix##_first_constants.data(),                                \
                                             prefix##_num_constants.data());                                 \
      }                                                                                                      \
                                                                                                             \
      void apply(ID3D11DeviceContext1& dc) noexcept                                                          \
      {                                                                                                      \
         dc.func_prefix##SetConstantBuffers1(0, prefix##_buffers.size(),                                     \
                                             reinterpret_cast<ID3D11Buffer**>(                               \
                                                prefix##_buffers.data()),                                    \
                                             prefix##_first_constants.data(),                                \
                                             prefix##_num_constants.data());                                 \
      }                                                                                                      \
   };

CREATE_SHADER_CBUFFER_TYPE(cs, CS)
CREATE_SHADER_CBUFFER_TYPE(vs, VS)
CREATE_SHADER_CBUFFER_TYPE(hs, HS)
CREATE_SHADER_CBUFFER_TYPE(ds, DS)
CREATE_SHADER_CBUFFER_TYPE(gs, GS)
CREATE_SHADER_CBUFFER_TYPE(ps, PS)

#define CREATE_SHADER_TYPE(prefix, func_prefix, shader_type)                   \
   struct prefix##_shader {                                                    \
      Com_ptr<shader_type> prefix##_shader;                                    \
                                                                               \
      void fill(ID3D11DeviceContext1& dc) noexcept                             \
      {                                                                        \
         dc.func_prefix##GetShader(prefix##_shader.clear_and_assign(),         \
                                   nullptr, nullptr);                          \
      }                                                                        \
                                                                               \
      void apply(ID3D11DeviceContext1& dc) noexcept                            \
      {                                                                        \
         dc.func_prefix##SetShader(prefix##_shader.get(), nullptr, 0);         \
      }                                                                        \
   };

CREATE_SHADER_TYPE(cs, CS, ID3D11ComputeShader)
CREATE_SHADER_TYPE(vs, VS, ID3D11VertexShader)
CREATE_SHADER_TYPE(hs, HS, ID3D11HullShader)
CREATE_SHADER_TYPE(ds, DS, ID3D11DomainShader)
CREATE_SHADER_TYPE(gs, GS, ID3D11GeometryShader)
CREATE_SHADER_TYPE(ps, PS, ID3D11PixelShader)

#define CREATE_SHADER_SAMPLER_TYPE(prefix, func_prefix)                                                       \
   struct prefix##_samplers {                                                                                 \
      std::array<Com_ptr<ID3D11SamplerState>, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT> prefix##_sampler_states; \
                                                                                                              \
      void fill(ID3D11DeviceContext1& dc) noexcept                                                            \
      {                                                                                                       \
         prefix##_sampler_states = {};                                                                        \
         dc.func_prefix##GetSamplers(0, prefix##_sampler_states.size(),                                       \
                                     reinterpret_cast<ID3D11SamplerState**>(                                  \
                                        prefix##_sampler_states.data()));                                     \
      }                                                                                                       \
                                                                                                              \
      void apply(ID3D11DeviceContext1& dc) noexcept                                                           \
      {                                                                                                       \
         dc.func_prefix##SetSamplers(0, prefix##_sampler_states.size(),                                       \
                                     reinterpret_cast<ID3D11SamplerState**>(                                  \
                                        prefix##_sampler_states.data()));                                     \
      }                                                                                                       \
   };

CREATE_SHADER_SAMPLER_TYPE(cs, CS)
CREATE_SHADER_SAMPLER_TYPE(vs, VS)
CREATE_SHADER_SAMPLER_TYPE(hs, HS)
CREATE_SHADER_SAMPLER_TYPE(ds, DS)
CREATE_SHADER_SAMPLER_TYPE(gs, GS)
CREATE_SHADER_SAMPLER_TYPE(ps, PS)

#define CREATE_SHADER_RESOURCE_TYPE(prefix, func_prefix)                                          \
   struct prefix##_resources {                                                                    \
      std::array<Com_ptr<ID3D11ShaderResourceView>, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT> \
         prefix##_resources;                                                                      \
                                                                                                  \
      void fill(ID3D11DeviceContext1& dc) noexcept                                                \
      {                                                                                           \
         prefix##_resources = {};                                                                 \
         dc.func_prefix##GetShaderResources(0, prefix##_resources.size(),                         \
                                            reinterpret_cast<ID3D11ShaderResourceView**>(         \
                                               prefix##_resources.data()));                       \
      }                                                                                           \
                                                                                                  \
      void apply(ID3D11DeviceContext1& dc) noexcept                                               \
      {                                                                                           \
         dc.func_prefix##GetShaderResources(0, prefix##_resources.size(),                         \
                                            reinterpret_cast<ID3D11ShaderResourceView**>(         \
                                               prefix##_resources.data()));                       \
      }                                                                                           \
   };

CREATE_SHADER_RESOURCE_TYPE(cs, CS)
CREATE_SHADER_RESOURCE_TYPE(vs, VS)
CREATE_SHADER_RESOURCE_TYPE(hs, HS)
CREATE_SHADER_RESOURCE_TYPE(ds, DS)
CREATE_SHADER_RESOURCE_TYPE(gs, GS)
CREATE_SHADER_RESOURCE_TYPE(ps, PS)

#undef CREATE_SHADER_CBUFFER_TYPE
#undef CREATE_SHADER_TYPE
#undef CREATE_SHADER_SAMPLER_TYPE
#undef CREATE_SHADER_RESOURCE_TYPE

struct cs_uavs {
   std::array<Com_ptr<ID3D11UnorderedAccessView>, D3D11_PS_CS_UAV_REGISTER_COUNT> cs_uavs;

   void fill(ID3D11DeviceContext1& dc) noexcept
   {
      cs_uavs = {};
      dc.CSGetUnorderedAccessViews(0, cs_uavs.size(),
                                   reinterpret_cast<ID3D11UnorderedAccessView**>(
                                      cs_uavs.data()));
   }

   void apply(ID3D11DeviceContext1& dc)
   {
      dc.CSSetUnorderedAccessViews(0, cs_uavs.size(),
                                   reinterpret_cast<ID3D11UnorderedAccessView**>(
                                      cs_uavs.data()),
                                   nullptr);
   }
};

struct ia_ibuffer {
   Com_ptr<ID3D11Buffer> ia_ibuffer;
   DXGI_FORMAT ia_ibuffer_format;
   UINT ia_ibuffer_offset;

   void fill(ID3D11DeviceContext1& dc) noexcept
   {
      dc.IAGetIndexBuffer(ia_ibuffer.clear_and_assign(), &ia_ibuffer_format,
                          &ia_ibuffer_offset);
   }

   void apply(ID3D11DeviceContext1& dc) noexcept
   {
      dc.IASetIndexBuffer(ia_ibuffer.get(), ia_ibuffer_format, ia_ibuffer_offset);
   }
};

struct ia_layout {
   Com_ptr<ID3D11InputLayout> ia_layout;

   void fill(ID3D11DeviceContext1& dc) noexcept
   {
      dc.IAGetInputLayout(ia_layout.clear_and_assign());
   }

   void apply(ID3D11DeviceContext1& dc) noexcept
   {
      dc.IASetInputLayout(ia_layout.get());
   }
};

struct ia_topology {
   D3D11_PRIMITIVE_TOPOLOGY ia_topology;

   void fill(ID3D11DeviceContext1& dc) noexcept
   {
      dc.IAGetPrimitiveTopology(&ia_topology);
   }

   void apply(ID3D11DeviceContext1& dc) noexcept
   {
      dc.IASetPrimitiveTopology(ia_topology);
   }
};

struct ia_vbuffers {
   std::array<Com_ptr<ID3D11Buffer>, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT> ia_vbuffers;
   std::array<UINT, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT> ia_vbuffer_strides;
   std::array<UINT, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT> ia_vbuffer_offsets;

   void fill(ID3D11DeviceContext1& dc) noexcept
   {
      dc.IAGetVertexBuffers(0, ia_vbuffers.size(),
                            reinterpret_cast<ID3D11Buffer**>(ia_vbuffers.data()),
                            ia_vbuffer_strides.data(), ia_vbuffer_offsets.data());
   }

   void apply(ID3D11DeviceContext1& dc) noexcept
   {
      dc.IASetVertexBuffers(0, ia_vbuffers.size(),
                            reinterpret_cast<ID3D11Buffer**>(ia_vbuffers.data()),
                            ia_vbuffer_strides.data(), ia_vbuffer_offsets.data());
   }
};

struct rs_scissors {
   UINT rs_scissor_count;
   std::array<D3D11_RECT, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE> rs_scissors;

   void fill(ID3D11DeviceContext1& dc) noexcept
   {
      rs_scissor_count = rs_scissors.size();
      dc.RSGetScissorRects(&rs_scissor_count, rs_scissors.data());
   }

   void apply(ID3D11DeviceContext1& dc) noexcept
   {
      dc.RSSetScissorRects(rs_scissor_count, rs_scissors.data());
   }
};

struct rs_state {
   Com_ptr<ID3D11RasterizerState> rs_state;

   void fill(ID3D11DeviceContext1& dc) noexcept
   {
      dc.RSGetState(rs_state.clear_and_assign());
   }

   void apply(ID3D11DeviceContext1& dc) noexcept
   {
      dc.RSSetState(rs_state.get());
   }
};

struct rs_viewports {
   UINT rs_viewport_count;
   std::array<D3D11_VIEWPORT, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE> rs_viewports;

   void fill(ID3D11DeviceContext1& dc) noexcept
   {
      rs_viewport_count = rs_viewports.size();
      dc.RSGetViewports(&rs_viewport_count, rs_viewports.data());
   }

   void apply(ID3D11DeviceContext1& dc) noexcept
   {
      dc.RSSetViewports(rs_viewport_count, rs_viewports.data());
   }
};

struct om_blend_state {
   Com_ptr<ID3D11BlendState> om_blend_state;
   std::array<float, 4> om_blend_factor;
   UINT om_sample_mask;

   void fill(ID3D11DeviceContext1& dc) noexcept
   {
      dc.OMGetBlendState(om_blend_state.clear_and_assign(),
                         om_blend_factor.data(), &om_sample_mask);
   }

   void apply(ID3D11DeviceContext1& dc) noexcept
   {
      dc.OMSetBlendState(om_blend_state.get(), om_blend_factor.data(), om_sample_mask);
   }
};

struct om_ds_state {
   Com_ptr<ID3D11DepthStencilState> om_ds_state;
   UINT om_stencil_ref;

   void fill(ID3D11DeviceContext1& dc) noexcept
   {
      dc.OMGetDepthStencilState(om_ds_state.clear_and_assign(), &om_stencil_ref);
   }

   void apply(ID3D11DeviceContext1& dc) noexcept
   {
      dc.OMSetDepthStencilState(om_ds_state.get(), om_stencil_ref);
   }
};

struct om_dsv_rtvs {
   std::array<Com_ptr<ID3D11RenderTargetView>, 8> om_rtvs;
   Com_ptr<ID3D11DepthStencilView> om_dsv;

   void fill(ID3D11DeviceContext1& dc) noexcept
   {
      om_rtvs = {};

      dc.OMGetRenderTargets(om_rtvs.size(),
                            reinterpret_cast<ID3D11RenderTargetView**>(om_rtvs.data()),
                            om_dsv.clear_and_assign());
   }

   void apply(ID3D11DeviceContext1& dc) noexcept
   {
      dc.OMSetRenderTargets(om_rtvs.size(),
                            reinterpret_cast<ID3D11RenderTargetView**>(om_rtvs.data()),
                            om_dsv.get());
   }
};

template<bool value, typename Type>
struct Optional_t : public Type {
};

template<typename Type>
struct Optional_t<false, Type> {
};

#pragma warning(push)
#pragma warning(disable : 4305) // 'if': truncation from 'unsigned __int64' to 'bool'

template<unsigned long long flags>
class Context_state_impl :
   // CS State
   Optional_t<flags & state_flags::cs_cbuffers, cs_cbuffers>,
   Optional_t<flags & state_flags::cs_samplers, cs_samplers>,
   Optional_t<flags & state_flags::cs_shader, cs_shader>,
   Optional_t<flags & state_flags::cs_resources, cs_resources>,
   Optional_t<flags & state_flags::cs_uavs, cs_uavs>,

   // IA State
   Optional_t<flags & state_flags::ia_ibuffer, ia_ibuffer>,
   Optional_t<flags & state_flags::ia_layout, ia_layout>,
   Optional_t<flags & state_flags::ia_topology, ia_topology>,
   Optional_t<flags & state_flags::ia_vbuffers, ia_vbuffers>,

   // VS State
   Optional_t<flags & state_flags::vs_cbuffers, vs_cbuffers>,
   Optional_t<flags & state_flags::vs_samplers, vs_samplers>,
   Optional_t<flags & state_flags::vs_shader, vs_shader>,
   Optional_t<flags & state_flags::vs_resources, vs_resources>,

   // HS State
   Optional_t<flags & state_flags::hs_cbuffers, hs_cbuffers>,
   Optional_t<flags & state_flags::hs_samplers, hs_samplers>,
   Optional_t<flags & state_flags::hs_shader, hs_shader>,
   Optional_t<flags & state_flags::hs_resources, hs_resources>,

   // DS State
   Optional_t<flags & state_flags::ds_cbuffers, ds_cbuffers>,
   Optional_t<flags & state_flags::ds_samplers, ds_samplers>,
   Optional_t<flags & state_flags::ds_shader, ds_shader>,
   Optional_t<flags & state_flags::ds_resources, ds_resources>,

   // GS State
   Optional_t<flags & state_flags::gs_cbuffers, gs_cbuffers>,
   Optional_t<flags & state_flags::gs_samplers, gs_samplers>,
   Optional_t<flags & state_flags::gs_shader, gs_shader>,
   Optional_t<flags & state_flags::gs_resources, gs_resources>,

   // RS State
   Optional_t<flags & state_flags::rs_scissors, rs_scissors>,
   Optional_t<flags & state_flags::rs_state, rs_state>,
   Optional_t<flags & state_flags::rs_viewports, rs_viewports>,

   // PS State
   Optional_t<flags & state_flags::ps_cbuffers, ps_cbuffers>,
   Optional_t<flags & state_flags::ps_samplers, ps_samplers>,
   Optional_t<flags & state_flags::ps_shader, ps_shader>,
   Optional_t<flags & state_flags::ps_resources, ps_resources>,

   // OM State
   Optional_t<flags & state_flags::om_blend_state, om_blend_state>,
   Optional_t<flags & state_flags::om_ds_state, om_ds_state>,
   Optional_t<flags & state_flags::om_dsv_rtvs, om_dsv_rtvs> {
public:
   void fill([[maybe_unused]] ID3D11DeviceContext1& dc) noexcept
   {
      // CS State

      if constexpr (flags & state_flags::cs_cbuffers) cs_cbuffers::fill(dc);
      if constexpr (flags & state_flags::cs_samplers) cs_samplers::fill(dc);
      if constexpr (flags & state_flags::cs_shader) cs_shader::fill(dc);
      if constexpr (flags & state_flags::cs_resources) cs_resources::fill(dc);
      if constexpr (flags & state_flags::cs_uavs) cs_uavs::fill(dc);

      // IA State

      if constexpr (flags & state_flags::ia_ibuffer) ia_ibuffer::fill(dc);
      if constexpr (flags & state_flags::ia_layout) ia_layout::fill(dc);
      if constexpr (flags & state_flags::ia_topology) ia_topology::fill(dc);
      if constexpr (flags & state_flags::ia_vbuffers) ia_vbuffers::fill(dc);

      // VS State

      if constexpr (flags & state_flags::vs_cbuffers) vs_cbuffers::fill(dc);
      if constexpr (flags & state_flags::vs_samplers) vs_samplers::fill(dc);
      if constexpr (flags & state_flags::vs_shader) vs_shader::fill(dc);
      if constexpr (flags & state_flags::vs_resources) vs_resources::fill(dc);

      // HS State

      if constexpr (flags & state_flags::hs_cbuffers) hs_cbuffers::fill(dc);
      if constexpr (flags & state_flags::hs_samplers) hs_samplers::fill(dc);
      if constexpr (flags & state_flags::hs_shader) hs_shader::fill(dc);
      if constexpr (flags & state_flags::hs_resources) hs_resources::fill(dc);

      // DS State

      if constexpr (flags & state_flags::ds_cbuffers) ds_cbuffers::fill(dc);
      if constexpr (flags & state_flags::ds_samplers) ds_samplers::fill(dc);
      if constexpr (flags & state_flags::ds_shader) ds_shader::fill(dc);
      if constexpr (flags & state_flags::ds_resources) ds_resources::fill(dc);

      // GS State

      if constexpr (flags & state_flags::gs_cbuffers) gs_cbuffers::fill(dc);
      if constexpr (flags & state_flags::gs_samplers) gs_samplers::fill(dc);
      if constexpr (flags & state_flags::gs_shader) gs_shader::fill(dc);
      if constexpr (flags & state_flags::gs_resources) gs_resources::fill(dc);

      // RS State

      if constexpr (flags & state_flags::rs_scissors) rs_scissors::fill(dc);
      if constexpr (flags & state_flags::rs_state) rs_state::fill(dc);
      if constexpr (flags & state_flags::rs_viewports) rs_viewports::fill(dc);

      // PS State

      if constexpr (flags & state_flags::ps_cbuffers) ps_cbuffers::fill(dc);
      if constexpr (flags & state_flags::ps_samplers) ps_samplers::fill(dc);
      if constexpr (flags & state_flags::ps_shader) ps_shader::fill(dc);
      if constexpr (flags & state_flags::ps_resources) ps_resources::fill(dc);

      // OM State

      if constexpr (flags & state_flags::om_blend_state)
         om_blend_state::fill(dc);
      if constexpr (flags & state_flags::om_ds_state) om_ds_state::fill(dc);
      if constexpr (flags & state_flags::om_dsv_rtvs) om_dsv_rtvs::fill(dc);
   }

   void apply([[maybe_unused]] ID3D11DeviceContext1& dc) noexcept
   {
      // CS State

      if constexpr (flags & state_flags::cs_cbuffers) cs_cbuffers::apply(dc);
      if constexpr (flags & state_flags::cs_samplers) cs_samplers::apply(dc);
      if constexpr (flags & state_flags::cs_shader) cs_shader::apply(dc);
      if constexpr (flags & state_flags::cs_resources) cs_resources::apply(dc);
      if constexpr (flags & state_flags::cs_uavs) cs_uavs::apply(dc);

      // IA State

      if constexpr (flags & state_flags::ia_ibuffer) ia_ibuffer::apply(dc);
      if constexpr (flags & state_flags::ia_layout) ia_layout::apply(dc);
      if constexpr (flags & state_flags::ia_topology) ia_topology::apply(dc);
      if constexpr (flags & state_flags::ia_vbuffers) ia_vbuffers::apply(dc);

      // VS State

      if constexpr (flags & state_flags::vs_cbuffers) vs_cbuffers::apply(dc);
      if constexpr (flags & state_flags::vs_samplers) vs_samplers::apply(dc);
      if constexpr (flags & state_flags::vs_shader) vs_shader::apply(dc);
      if constexpr (flags & state_flags::vs_resources) vs_resources::apply(dc);

      // HS State

      if constexpr (flags & state_flags::hs_cbuffers) hs_cbuffers::apply(dc);
      if constexpr (flags & state_flags::hs_samplers) hs_samplers::apply(dc);
      if constexpr (flags & state_flags::hs_shader) hs_shader::apply(dc);
      if constexpr (flags & state_flags::hs_resources) hs_resources::apply(dc);

      // DS State

      if constexpr (flags & state_flags::ds_cbuffers) ds_cbuffers::apply(dc);
      if constexpr (flags & state_flags::ds_samplers) ds_samplers::apply(dc);
      if constexpr (flags & state_flags::ds_shader) ds_shader::apply(dc);
      if constexpr (flags & state_flags::ds_resources) ds_resources::apply(dc);

      // GS State

      if constexpr (flags & state_flags::gs_cbuffers) gs_cbuffers::apply(dc);
      if constexpr (flags & state_flags::gs_samplers) gs_samplers::apply(dc);
      if constexpr (flags & state_flags::gs_shader) gs_shader::apply(dc);
      if constexpr (flags & state_flags::gs_resources) gs_resources::apply(dc);

      // RS State

      if constexpr (flags & state_flags::rs_scissors) rs_scissors::apply(dc);
      if constexpr (flags & state_flags::rs_state) rs_state::apply(dc);
      if constexpr (flags & state_flags::rs_viewports) rs_viewports::apply(dc);

      // PS State

      if constexpr (flags & state_flags::ps_cbuffers) ps_cbuffers::apply(dc);
      if constexpr (flags & state_flags::ps_samplers) ps_samplers::apply(dc);
      if constexpr (flags & state_flags::ps_shader) ps_shader::apply(dc);
      if constexpr (flags & state_flags::ps_resources) ps_resources::apply(dc);

      // OM State

      if constexpr (flags & state_flags::om_blend_state)
         om_blend_state::apply(dc);
      if constexpr (flags & state_flags::om_ds_state) om_ds_state::apply(dc);
      if constexpr (flags & state_flags::om_dsv_rtvs) om_dsv_rtvs::apply(dc);
   }
};

#pragma warning(pop)

}

template<unsigned long long flags>
class Context_state_guard : private detail::Context_state_impl<flags> {
public:
   static_assert(
      flags, "No state flags specified, this makes the state guard pointless.");

   Context_state_guard(ID3D11DeviceContext1& dc) noexcept : _dc{dc}
   {
      this->fill(_dc);
   }

   ~Context_state_guard() noexcept
   {
      this->apply(_dc);
   }

   Context_state_guard() = delete;
   Context_state_guard(const Context_state_guard&) = delete;
   Context_state_guard& operator=(const Context_state_guard&) = delete;
   Context_state_guard(Context_state_guard&&) = delete;
   Context_state_guard& operator=(Context_state_guard&&) = delete;

private:
   ID3D11DeviceContext1& _dc;
};
}
