#pragma once

#include "../core/shader_patch.hpp"
#include "../logger.hpp"
#include "com_ptr.hpp"
#include "com_ref.hpp"
#include "render_state_manager.hpp"
#include "surface_backbuffer.hpp"
#include "surface_depthstencil.hpp"
#include "texture_stage_state_manager.hpp"

#include <array>

#include <d3d9.h>

namespace sp::d3d9 {

class Device final : public IDirect3DDevice9 {
public:
   static Com_ptr<Device> create(IDirect3D9& direct3d9, IDXGIAdapter2& adapter,
                                 const HWND window, const UINT width,
                                 const UINT height) noexcept;

   Device(const Device&) = delete;
   Device& operator=(const Device&) = delete;

   Device(Device&&) = delete;
   Device& operator=(Device&&) = delete;

   HRESULT __stdcall QueryInterface(const IID& iid, void** object) noexcept override;
   ULONG __stdcall AddRef() noexcept override;
   ULONG __stdcall Release() noexcept override;

   HRESULT __stdcall TestCooperativeLevel() noexcept override;

   UINT __stdcall GetAvailableTextureMem() noexcept override;

   HRESULT __stdcall GetDirect3D(IDirect3D9** d3d9) noexcept override;

   HRESULT __stdcall GetDeviceCaps(D3DCAPS9* caps) noexcept override;

   HRESULT __stdcall GetDisplayMode(UINT swap_chain, D3DDISPLAYMODE* mode) noexcept override;

   HRESULT __stdcall SetCursorProperties(UINT, UINT, IDirect3DSurface9*) noexcept override
   {
      return S_OK;
   }

   void __stdcall SetCursorPosition(int x, int y, DWORD flags) noexcept override;

   BOOL __stdcall ShowCursor(BOOL show) noexcept;

   HRESULT __stdcall Reset(D3DPRESENT_PARAMETERS* presentation_parameters) noexcept override;

   HRESULT __stdcall Present(const RECT* source_rect, const RECT* dest_rect,
                             HWND dest_window_override,
                             const RGNDATA* dirty_region) noexcept override;

   HRESULT __stdcall GetBackBuffer(UINT swap_chain, UINT back_buffer_index,
                                   D3DBACKBUFFER_TYPE type,
                                   IDirect3DSurface9** back_buffer) noexcept override;

   void __stdcall SetGammaRamp(UINT, DWORD, const D3DGAMMARAMP*) noexcept override
   {
   }

   HRESULT __stdcall CreateTexture(UINT width, UINT height, UINT levels,
                                   DWORD usage, D3DFORMAT format, D3DPOOL pool,
                                   IDirect3DTexture9** texture,
                                   HANDLE* shared_handle) noexcept override;

   HRESULT __stdcall CreateVolumeTexture(UINT width, UINT height, UINT depth,
                                         UINT levels, DWORD usage,
                                         D3DFORMAT format, D3DPOOL pool,
                                         IDirect3DVolumeTexture9** volume_texture,
                                         HANDLE* shared_handle) noexcept override;

   HRESULT __stdcall CreateCubeTexture(UINT edge_length, UINT levels, DWORD usage,
                                       D3DFORMAT format, D3DPOOL pool,
                                       IDirect3DCubeTexture9** cube_texture,
                                       HANDLE* shared_handle) noexcept override;

   HRESULT __stdcall CreateVertexBuffer(UINT length, DWORD usage, DWORD fvf,
                                        D3DPOOL pool,
                                        IDirect3DVertexBuffer9** vertex_buffer,
                                        HANDLE* shared_handle) noexcept override;

   HRESULT __stdcall CreateIndexBuffer(UINT length, DWORD usage,
                                       D3DFORMAT format, D3DPOOL pool,
                                       IDirect3DIndexBuffer9** index_buffer,
                                       HANDLE* shared_handle) noexcept override;

   HRESULT __stdcall CreateDepthStencilSurface(
      UINT width, UINT height, D3DFORMAT format,
      D3DMULTISAMPLE_TYPE multi_sample, DWORD multi_sample_quality, BOOL discard,
      IDirect3DSurface9** surface, HANDLE* shared_handle) noexcept override;

   HRESULT __stdcall GetRenderTargetData(IDirect3DSurface9* render_target,
                                         IDirect3DSurface9* dest_surface) noexcept override;

   HRESULT __stdcall StretchRect(IDirect3DSurface9* source_surface,
                                 const RECT* source_rect,
                                 IDirect3DSurface9* dest_surface, const RECT* dest_rect,
                                 D3DTEXTUREFILTERTYPE filter) noexcept override;

   HRESULT __stdcall ColorFill(IDirect3DSurface9* surface, const RECT* rect,
                               D3DCOLOR color) noexcept override;

   HRESULT __stdcall CreateOffscreenPlainSurface(UINT width, UINT height,
                                                 D3DFORMAT format, D3DPOOL pool,
                                                 IDirect3DSurface9** surface,
                                                 HANDLE* shared_handle) noexcept override;

   HRESULT __stdcall SetRenderTarget(DWORD rendertarget_index,
                                     IDirect3DSurface9* rendertarget) noexcept override;

   HRESULT __stdcall GetRenderTarget(DWORD rendertarget_index,
                                     IDirect3DSurface9** rendertarget) noexcept override;

   HRESULT __stdcall SetDepthStencilSurface(IDirect3DSurface9* new_z_stencil) noexcept override;

   HRESULT __stdcall GetDepthStencilSurface(IDirect3DSurface9** z_stencil_surface) noexcept override;

   HRESULT __stdcall BeginScene() noexcept override;

   HRESULT __stdcall EndScene() noexcept override;

   HRESULT __stdcall Clear(DWORD count, const D3DRECT* rects, DWORD flags,
                           D3DCOLOR color, float z, DWORD stencil) noexcept override;

   HRESULT __stdcall SetTransform(D3DTRANSFORMSTATETYPE state,
                                  const D3DMATRIX* matrix) noexcept override;

   HRESULT __stdcall GetTransform(D3DTRANSFORMSTATETYPE state,
                                  D3DMATRIX* matrix) noexcept override;

   HRESULT __stdcall SetViewport(const D3DVIEWPORT9* viewport) noexcept override;

   HRESULT __stdcall GetViewport(D3DVIEWPORT9* viewport) noexcept override;

   HRESULT __stdcall SetRenderState(D3DRENDERSTATETYPE state, DWORD value) noexcept override;

   HRESULT __stdcall GetRenderState(D3DRENDERSTATETYPE state,
                                    DWORD* value) noexcept override;

   HRESULT __stdcall GetTexture(DWORD stage,
                                IDirect3DBaseTexture9** texture) noexcept override;

   HRESULT __stdcall SetTexture(DWORD stage,
                                IDirect3DBaseTexture9* texture) noexcept override;

   HRESULT __stdcall GetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE state,
                                          DWORD* value) noexcept override;

   HRESULT __stdcall SetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE state,
                                          DWORD value) noexcept override;

   HRESULT __stdcall GetSamplerState(DWORD sampler, D3DSAMPLERSTATETYPE state,
                                     DWORD* value) noexcept override;

   HRESULT __stdcall SetSamplerState(DWORD sampler, D3DSAMPLERSTATETYPE state,
                                     DWORD value) noexcept override;

   HRESULT __stdcall DrawPrimitive(D3DPRIMITIVETYPE primitive_type, UINT start_vertex,
                                   UINT primitive_count) noexcept override;

   HRESULT __stdcall DrawIndexedPrimitive(D3DPRIMITIVETYPE primitive_type,
                                          INT base_vertex_index, UINT min_vertex_index,
                                          UINT num_vertices, UINT start_Index,
                                          UINT prim_count) noexcept override;

   HRESULT __stdcall CreateVertexDeclaration(const D3DVERTEXELEMENT9* vertex_elements,
                                             IDirect3DVertexDeclaration9** decl) noexcept override;

   HRESULT __stdcall SetVertexDeclaration(IDirect3DVertexDeclaration9* decl) noexcept override;

   HRESULT __stdcall SetFVF(DWORD fvf) noexcept override;

   HRESULT __stdcall GetFVF(DWORD* fvf) noexcept override;

   HRESULT __stdcall CreateVertexShader(const DWORD* function,
                                        IDirect3DVertexShader9** shader) noexcept override;

   HRESULT __stdcall SetVertexShader(IDirect3DVertexShader9* shader) noexcept override;

   HRESULT __stdcall SetVertexShaderConstantF(UINT start_register,
                                              const float* constant_data,
                                              UINT vector4f_count) noexcept override;

   HRESULT __stdcall SetStreamSource(UINT stream_number,
                                     IDirect3DVertexBuffer9* stream_data,
                                     UINT offset_in_bytes, UINT stride) noexcept override;

   HRESULT __stdcall SetIndices(IDirect3DIndexBuffer9* index_data) noexcept override;

   HRESULT __stdcall CreatePixelShader(const DWORD* function,
                                       IDirect3DPixelShader9** shader) noexcept override;

   HRESULT __stdcall SetPixelShader(IDirect3DPixelShader9* shader) noexcept override;

   HRESULT __stdcall SetPixelShaderConstantF(UINT start_register,
                                             const float* constant_data,
                                             UINT vector4f_count) noexcept override;

   HRESULT __stdcall CreateQuery(D3DQUERYTYPE type,
                                 IDirect3DQuery9** query) noexcept override;

   [[deprecated("unimplemented")]] HRESULT __stdcall CreateRenderTarget(
      UINT, UINT, D3DFORMAT, D3DMULTISAMPLE_TYPE, DWORD, BOOL,
      IDirect3DSurface9**, HANDLE*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemente"
                "d")]] HRESULT __stdcall EvictManagedResources() noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetCreationParameters(
      D3DDEVICE_CREATION_PARAMETERS*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall CreateAdditionalSwapChain(
      D3DPRESENT_PARAMETERS*, IDirect3DSwapChain9**) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetSwapChain(
      UINT, IDirect3DSwapChain9**) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemente"
                "d")]] UINT __stdcall GetNumberOfSwapChains() noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetRasterStatus(
      UINT, D3DRASTER_STATUS*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall SetDialogBoxMode(BOOL) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] void __stdcall GetGammaRamp(UINT, D3DGAMMARAMP*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall UpdateSurface(
      IDirect3DSurface9*, const RECT*, IDirect3DSurface9*, const POINT*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall UpdateTexture(
      IDirect3DBaseTexture9*, IDirect3DBaseTexture9*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetFrontBufferData(
      UINT, IDirect3DSurface9*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall MultiplyTransform(
      D3DTRANSFORMSTATETYPE, const D3DMATRIX*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall SetMaterial(const D3DMATERIAL9*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetMaterial(D3DMATERIAL9*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall SetLight(DWORD,
                                                              const D3DLIGHT9*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetLight(DWORD, D3DLIGHT9*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall LightEnable(DWORD, BOOL) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetLightEnable(DWORD, BOOL*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall SetClipPlane(DWORD,
                                                                  const float*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetClipPlane(DWORD, float*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall CreateStateBlock(
      D3DSTATEBLOCKTYPE, IDirect3DStateBlock9**) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated(
      "unimplemented")]] HRESULT __stdcall BeginStateBlock() noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall EndStateBlock(
      IDirect3DStateBlock9**) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall SetClipStatus(const D3DCLIPSTATUS9*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetClipStatus(D3DCLIPSTATUS9*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall ValidateDevice(DWORD*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall SetPaletteEntries(
      UINT, const PALETTEENTRY*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetPaletteEntries(UINT,
                                                                       PALETTEENTRY*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall SetCurrentTexturePalette(UINT) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetCurrentTexturePalette(UINT*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall SetScissorRect(const RECT*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetScissorRect(RECT*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated(
      "unimplemented")]] HRESULT __stdcall SetSoftwareVertexProcessing(BOOL) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemente"
                "d")]] BOOL __stdcall GetSoftwareVertexProcessing() noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall SetNPatchMode(float) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated(
      "unimplemented")]] float __stdcall GetNPatchMode() noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall DrawPrimitiveUP(
      D3DPRIMITIVETYPE, UINT, const void*, UINT) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall DrawIndexedPrimitiveUP(
      D3DPRIMITIVETYPE, UINT, UINT, UINT, const void*, D3DFORMAT, const void*,
      UINT) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall ProcessVertices(
      UINT, UINT, UINT, IDirect3DVertexBuffer9*, IDirect3DVertexDeclaration9*,
      DWORD) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetVertexDeclaration(
      IDirect3DVertexDeclaration9**) noexcept
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetVertexShader(
      IDirect3DVertexShader9**) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetVertexShaderConstantF(
      UINT, float*, UINT) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall SetVertexShaderConstantI(
      UINT, const int*, UINT) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetVertexShaderConstantI(
      UINT, int*, UINT) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall SetVertexShaderConstantB(
      UINT, const BOOL*, UINT) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetVertexShaderConstantB(
      UINT, BOOL*, UINT) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetStreamSource(
      UINT, IDirect3DVertexBuffer9**, UINT*, UINT*) noexcept
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall SetStreamSourceFreq(UINT, UINT) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetStreamSourceFreq(UINT, UINT*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetIndices(IDirect3DIndexBuffer9**) noexcept
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetPixelShader(
      IDirect3DPixelShader9**) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetPixelShaderConstantF(
      UINT, float*, UINT) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall SetPixelShaderConstantI(
      UINT, const int*, UINT) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetPixelShaderConstantI(
      UINT, int*, UINT) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall SetPixelShaderConstantB(
      UINT, const BOOL*, UINT) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetPixelShaderConstantB(
      UINT, BOOL*, UINT) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall DrawRectPatch(
      UINT, const float*, const D3DRECTPATCH_INFO*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall DrawTriPatch(
      UINT, const float*, const D3DTRIPATCH_INFO*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall DeletePatch(UINT) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

private:
   Device(IDirect3D9& direct3d9, IDXGIAdapter2& adapter, const HWND window,
          const UINT width, const UINT height) noexcept;
   ~Device() = default;

   auto create_texture2d_managed(const UINT width, const UINT height,
                                 const UINT mip_levels, const D3DFORMAT d3d_format) noexcept
      -> Com_ptr<IDirect3DTexture9>;

   auto create_texture2d_rendertarget(const UINT width, const UINT height) noexcept
      -> Com_ptr<IDirect3DTexture9>;

   auto create_texture3d_managed(const UINT width, const UINT height,
                                 const UINT depth, const UINT mip_levels,
                                 const D3DFORMAT d3d_format) noexcept
      -> Com_ptr<IDirect3DVolumeTexture9>;

   auto create_texturecube_managed(const UINT width, const UINT mip_levels,
                                   const D3DFORMAT d3d_format) noexcept
      -> Com_ptr<IDirect3DCubeTexture9>;

   auto create_vertex_buffer(const UINT size, const bool dynamic) noexcept
      -> Com_ptr<IDirect3DVertexBuffer9>;

   auto create_index_buffer(const UINT size, const bool dynamic) noexcept
      -> Com_ptr<IDirect3DIndexBuffer9>;

   auto create_vertex_declaration(const D3DVERTEXELEMENT9* const vertex_elements) noexcept
      -> Com_ptr<IDirect3DVertexDeclaration9>;

   void draw_common(const D3DPRIMITIVETYPE primitive_type) noexcept;

   const Com_ref<IDirect3D9> _direct3d9;
   const Com_ref<IDXGIAdapter2> _adapter;
   const HWND _window;

   core::Shader_patch _shader_patch;
   Render_state_manager _render_state_manager;

   D3DPRIMITIVETYPE _last_primitive_type;
   bool _fixed_func_active = true;

   std::uint16_t _width;
   std::uint16_t _height;

   Com_ptr<IUnknown> _backbuffer{
      Surface_backbuffer::create(_shader_patch.get_back_buffer(), _width, _height)};

   Com_ptr<IUnknown> _rendertarget{_backbuffer};

   Com_ptr<IUnknown> _depthstencil{
      Surface_depthstencil::create(core::Game_depthstencil::nearscene, _width, _height)};

   D3DVIEWPORT9 _viewport{0, 0, _width, _height, 0.0f, 1.0f};
   Texture_stage_state_manager _texture_stage_manager{_shader_patch};

   ULONG _ref_count = 1;
};

constexpr auto device_caps = []() -> D3DCAPS9 {
   D3DCAPS9 caps{};
   caps.DeviceType = D3DDEVTYPE_HAL;
   caps.AdapterOrdinal = 0;
   caps.Caps = 131072;
   caps.Caps2 = 3758227456;
   caps.Caps3 = 928;
   caps.PresentationIntervals = 2147483663;
   caps.CursorCaps = 1;
   caps.DevCaps = 1818352;
   caps.PrimitiveMiscCaps = 3133170;
   caps.RasterCaps = 124985745;
   caps.ZCmpCaps = 255;
   caps.SrcBlendCaps = 16383;
   caps.DestBlendCaps = 16383;
   caps.AlphaCmpCaps = 255;
   caps.ShadeCaps = 541192;
   caps.TextureCaps = 126149;
   caps.TextureFilterCaps = 50530048;
   caps.CubeTextureFilterCaps = 50529024;
   caps.VolumeTextureFilterCaps = 50529024;
   caps.TextureAddressCaps = 63;
   caps.VolumeTextureAddressCaps = 63;
   caps.LineCaps = 31;
   caps.MaxTextureWidth = 16384;
   caps.MaxTextureHeight = 16384;
   caps.MaxVolumeExtent = 2048;
   caps.MaxTextureRepeat = 8192;
   caps.MaxTextureAspectRatio = 16384;
   caps.MaxAnisotropy = 16;
   caps.MaxVertexW = 1.00000000e+10;
   caps.GuardBandLeft = -100000000.;
   caps.GuardBandTop = -100000000.;
   caps.GuardBandRight = 100000000.;
   caps.GuardBandBottom = 100000000.;
   caps.ExtentsAdjust = 0.000000000;
   caps.StencilCaps = 511;
   caps.FVFCaps = 1572872;
   caps.TextureOpCaps = 67043327;
   caps.MaxTextureBlendStages = 4;
   caps.MaxSimultaneousTextures = 8;
   caps.VertexProcessingCaps = 315;
   caps.MaxActiveLights = 8;
   caps.MaxUserClipPlanes = 8;
   caps.MaxVertexBlendMatrices = 4;
   caps.MaxVertexBlendMatrixIndex = 0;
   caps.MaxPointSize = 8192.00000;
   caps.MaxPrimitiveCount = 16777215;
   caps.MaxVertexIndex = 16777215;
   caps.MaxStreams = 16;
   caps.MaxStreamStride = 255;
   caps.VertexShaderVersion = 4294836992;
   caps.MaxVertexShaderConst = 256;
   caps.PixelShaderVersion = 4294902528;
   caps.PixelShader1xMaxValue = 65504.0000;
   caps.DevCaps2 = 81;
   caps.MaxNpatchTessellationLevel = 0.000000000;
   caps.Reserved5 = 0;
   caps.MasterAdapterOrdinal = 0;
   caps.AdapterOrdinalInGroup = 0;
   caps.NumberOfAdaptersInGroup = 1;
   caps.DeclTypes = 783;
   caps.NumSimultaneousRTs = 4;
   caps.StretchRectFilterCaps = 50332416;
   caps.VS20Caps.Caps = 1;
   caps.VS20Caps.DynamicFlowControlDepth = 24;
   caps.VS20Caps.NumTemps = 32;
   caps.VS20Caps.StaticFlowControlDepth = 4;
   caps.PS20Caps.Caps = 31;
   caps.PS20Caps.DynamicFlowControlDepth = 24;
   caps.PS20Caps.NumTemps = 32;
   caps.PS20Caps.StaticFlowControlDepth = 4;
   caps.PS20Caps.NumInstructionSlots = 512;
   caps.VertexTextureFilterCaps = 50530048;
   caps.MaxVShaderInstructionsExecuted = 65535;
   caps.MaxPShaderInstructionsExecuted = 65535;
   caps.MaxVertexShader30InstructionSlots = 4096;
   caps.MaxPixelShader30InstructionSlots = 4096;

   return caps;
}();
}
