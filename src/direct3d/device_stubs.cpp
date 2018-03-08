
#include "device.hpp"

namespace sp::direct3d {

HRESULT Device::QueryInterface(const IID& iid, void** object) noexcept
{
   return _device->QueryInterface(iid, object);
}

UINT Device::GetAvailableTextureMem() noexcept
{
   return _device->GetAvailableTextureMem();
}

HRESULT Device::EvictManagedResources() noexcept
{
   return _device->EvictManagedResources();
}

HRESULT Device::GetDirect3D(IDirect3D9** d3d9) noexcept
{
   return _device->GetDirect3D(d3d9);
}

HRESULT Device::GetDeviceCaps(D3DCAPS9* caps) noexcept
{
   return _device->GetDeviceCaps(caps);
}

HRESULT Device::GetDisplayMode(UINT swap_chain, D3DDISPLAYMODE* mode) noexcept
{
   return _device->GetDisplayMode(swap_chain, mode);
}

HRESULT Device::GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS* parameters) noexcept
{
   return _device->GetCreationParameters(parameters);
}

HRESULT Device::SetCursorProperties(UINT x_hot_spot, UINT y_hot_spot,
                                    IDirect3DSurface9* cursor_bitmap) noexcept
{
   return _device->SetCursorProperties(x_hot_spot, y_hot_spot, cursor_bitmap);
}

void Device::SetCursorPosition(int x, int y, DWORD flags) noexcept
{
   return _device->SetCursorPosition(x, y, flags);
}

BOOL Device::ShowCursor(BOOL show) noexcept
{
   return _device->ShowCursor(show);
}

HRESULT Device::CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS* presentation_parameters,
                                          IDirect3DSwapChain9** swap_chain) noexcept
{
   return _device->CreateAdditionalSwapChain(presentation_parameters, swap_chain);
}

HRESULT Device::GetSwapChain(UINT swap_chain_index, IDirect3DSwapChain9** swap_chain) noexcept
{
   return _device->GetSwapChain(swap_chain_index, swap_chain);
}

UINT Device::GetNumberOfSwapChains() noexcept
{
   return _device->GetNumberOfSwapChains();
}

HRESULT Device::GetBackBuffer(UINT swap_chain, UINT back_buffer_index,
                              D3DBACKBUFFER_TYPE type,
                              IDirect3DSurface9** back_buffer) noexcept
{
   return _device->GetBackBuffer(swap_chain, back_buffer_index, type, back_buffer);
}

HRESULT Device::GetRasterStatus(UINT swap_chain, D3DRASTER_STATUS* raster_status) noexcept
{
   return _device->GetRasterStatus(swap_chain, raster_status);
}

HRESULT Device::SetDialogBoxMode(BOOL enable_dialogs) noexcept
{
   return _device->SetDialogBoxMode(enable_dialogs);
}

void Device::SetGammaRamp(UINT swap_chain_index, DWORD flags,
                          const D3DGAMMARAMP* ramp) noexcept
{
   return _device->SetGammaRamp(swap_chain_index, flags, ramp);
}

void Device::GetGammaRamp(UINT swap_chain_index, D3DGAMMARAMP* ramp) noexcept
{
   return _device->GetGammaRamp(swap_chain_index, ramp);
}
HRESULT Device::CreateVolumeTexture(UINT width, UINT height, UINT depth, UINT levels,
                                    DWORD usage, D3DFORMAT format, D3DPOOL pool,
                                    IDirect3DVolumeTexture9** volume_texture,
                                    HANDLE* shared_handle) noexcept
{
   return _device->CreateVolumeTexture(width, height, depth, levels, usage, format,
                                       pool, volume_texture, shared_handle);
}

HRESULT Device::CreateCubeTexture(UINT edge_length, UINT levels, DWORD usage,
                                  D3DFORMAT format, D3DPOOL pool,
                                  IDirect3DCubeTexture9** cube_texture,
                                  HANDLE* shared_handle) noexcept
{
   return _device->CreateCubeTexture(edge_length, levels, usage, format, pool,
                                     cube_texture, shared_handle);
}

HRESULT Device::CreateVertexBuffer(UINT length, DWORD usage, DWORD fvf, D3DPOOL pool,
                                   IDirect3DVertexBuffer9** vertex_buffer,
                                   HANDLE* shared_handle) noexcept
{
   return _device->CreateVertexBuffer(length, usage, fvf, pool, vertex_buffer,
                                      shared_handle);
}

HRESULT Device::CreateRenderTarget(UINT width, UINT height, D3DFORMAT format,
                                   D3DMULTISAMPLE_TYPE multi_sample,
                                   DWORD multisample_quality, BOOL lockable,
                                   IDirect3DSurface9** surface,
                                   HANDLE* shared_handle) noexcept
{
   return _device->CreateRenderTarget(width, height, format, multi_sample,
                                      multisample_quality, lockable, surface,
                                      shared_handle);
}

HRESULT Device::CreateIndexBuffer(UINT length, DWORD usage, D3DFORMAT format,
                                  D3DPOOL pool, IDirect3DIndexBuffer9** index_buffer,
                                  HANDLE* shared_handle) noexcept
{
   return _device->CreateIndexBuffer(length, usage, format, pool, index_buffer,
                                     shared_handle);
}

HRESULT Device::UpdateSurface(IDirect3DSurface9* source_surface, const RECT* source_rect,
                              IDirect3DSurface9* destination_surface,
                              const POINT* dest_point) noexcept
{
   return _device->UpdateSurface(source_surface, source_rect,
                                 destination_surface, dest_point);
}

HRESULT Device::UpdateTexture(IDirect3DBaseTexture9* source_texture,
                              IDirect3DBaseTexture9* destination_texture) noexcept
{
   return _device->UpdateTexture(source_texture, destination_texture);
}

HRESULT Device::GetRenderTargetData(IDirect3DSurface9* render_target,
                                    IDirect3DSurface9* dest_surface) noexcept
{
   return _device->GetRenderTargetData(render_target, dest_surface);
}

HRESULT Device::GetFrontBufferData(UINT swap_chain_index,
                                   IDirect3DSurface9* dest_surface) noexcept
{
   return _device->GetFrontBufferData(swap_chain_index, dest_surface);
}

HRESULT Device::StretchRect(IDirect3DSurface9* source_surface,
                            const RECT* source_rect, IDirect3DSurface9* dest_surface,
                            const RECT* dest_rect, D3DTEXTUREFILTERTYPE filter) noexcept
{
   return _device->StretchRect(source_surface, source_rect, dest_surface,
                               dest_rect, filter);
}

HRESULT Device::ColorFill(IDirect3DSurface9* surface, const RECT* rect,
                          D3DCOLOR color) noexcept
{
   return _device->ColorFill(surface, rect, color);
}

HRESULT Device::CreateOffscreenPlainSurface(UINT width, UINT height,
                                            D3DFORMAT format, D3DPOOL pool,
                                            IDirect3DSurface9** surface,
                                            HANDLE* shared_handle) noexcept
{
   return _device->CreateOffscreenPlainSurface(width, height, format, pool,
                                               surface, shared_handle);
}

HRESULT Device::GetRenderTarget(DWORD render_target_index,
                                IDirect3DSurface9** render_target) noexcept
{
   return _device->GetRenderTarget(render_target_index, render_target);
}

HRESULT Device::SetDepthStencilSurface(IDirect3DSurface9* new_z_stencil) noexcept
{
   return _device->SetDepthStencilSurface(new_z_stencil);
}

HRESULT Device::GetDepthStencilSurface(IDirect3DSurface9** z_stencil_surface) noexcept
{
   return _device->GetDepthStencilSurface(z_stencil_surface);
}

HRESULT Device::BeginScene() noexcept
{
   return _device->BeginScene();
}

HRESULT Device::EndScene() noexcept
{
   return _device->EndScene();
}

HRESULT Device::Clear(DWORD count, const D3DRECT* rects, DWORD flags,
                      D3DCOLOR color, float z, DWORD stencil) noexcept
{
   return _device->Clear(count, rects, flags, color, z, stencil);
}

HRESULT Device::SetTransform(D3DTRANSFORMSTATETYPE state, const D3DMATRIX* matrix) noexcept
{
   return _device->SetTransform(state, matrix);
}

HRESULT Device::GetTransform(D3DTRANSFORMSTATETYPE state, D3DMATRIX* matrix) noexcept
{
   return _device->GetTransform(state, matrix);
}

HRESULT Device::MultiplyTransform(D3DTRANSFORMSTATETYPE state,
                                  const D3DMATRIX* matrix) noexcept
{
   return _device->MultiplyTransform(state, matrix);
}

HRESULT Device::SetViewport(const D3DVIEWPORT9* viewport) noexcept
{
   return _device->SetViewport(viewport);
}

HRESULT Device::GetViewport(D3DVIEWPORT9* viewport) noexcept
{
   return _device->GetViewport(viewport);
}

HRESULT Device::SetMaterial(const D3DMATERIAL9* material) noexcept
{
   return _device->SetMaterial(material);
}

HRESULT Device::GetMaterial(D3DMATERIAL9* material) noexcept
{
   return _device->GetMaterial(material);
}

HRESULT Device::SetLight(DWORD index, const D3DLIGHT9* light) noexcept
{
   return _device->SetLight(index, light);
}

HRESULT Device::GetLight(DWORD index, D3DLIGHT9* light) noexcept
{
   return _device->GetLight(index, light);
}

HRESULT Device::LightEnable(DWORD index, BOOL enabled) noexcept
{
   return _device->LightEnable(index, enabled);
}

HRESULT Device::GetLightEnable(DWORD index, BOOL* enabled) noexcept
{
   return _device->GetLightEnable(index, enabled);
}

HRESULT Device::SetClipPlane(DWORD index, const float* plane) noexcept
{
   return _device->SetClipPlane(index, plane);
}

HRESULT Device::GetClipPlane(DWORD index, float* plane) noexcept
{
   return _device->GetClipPlane(index, plane);
}

HRESULT Device::GetRenderState(D3DRENDERSTATETYPE state, DWORD* value) noexcept
{
   return _device->GetRenderState(state, value);
}

HRESULT Device::CreateStateBlock(D3DSTATEBLOCKTYPE type,
                                 IDirect3DStateBlock9** state_block) noexcept
{
   return _device->CreateStateBlock(type, state_block);
}

HRESULT Device::BeginStateBlock() noexcept
{
   return _device->BeginStateBlock();
}

HRESULT Device::EndStateBlock(IDirect3DStateBlock9** state_block) noexcept
{
   return _device->EndStateBlock(state_block);
}

HRESULT Device::SetClipStatus(const D3DCLIPSTATUS9* clip_status) noexcept
{
   return _device->SetClipStatus(clip_status);
}

HRESULT Device::GetClipStatus(D3DCLIPSTATUS9* clip_status) noexcept
{
   return _device->GetClipStatus(clip_status);
}

HRESULT Device::GetTexture(DWORD stage, IDirect3DBaseTexture9** texture) noexcept
{
   return _device->GetTexture(stage, texture);
}

HRESULT Device::GetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE type,
                                     DWORD* value) noexcept
{
   return _device->GetTextureStageState(stage, type, value);
}

HRESULT Device::SetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE type,
                                     DWORD value) noexcept
{
   return _device->SetTextureStageState(stage, type, value);
}

HRESULT Device::GetSamplerState(DWORD sampler, D3DSAMPLERSTATETYPE type, DWORD* value) noexcept
{
   return _device->GetSamplerState(sampler, type, value);
}

HRESULT Device::SetSamplerState(DWORD sampler, D3DSAMPLERSTATETYPE type, DWORD value) noexcept
{
   return _device->SetSamplerState(sampler, type, value);
}

HRESULT Device::ValidateDevice(DWORD* num_passes) noexcept
{
   return _device->ValidateDevice(num_passes);
}

HRESULT Device::SetPaletteEntries(UINT palette_number, const PALETTEENTRY* entries) noexcept
{
   return _device->SetPaletteEntries(palette_number, entries);
}

HRESULT Device::GetPaletteEntries(UINT palette_number, PALETTEENTRY* entries) noexcept
{
   return _device->GetPaletteEntries(palette_number, entries);
}

HRESULT Device::SetCurrentTexturePalette(UINT palette_number) noexcept
{
   return _device->SetCurrentTexturePalette(palette_number);
}

HRESULT Device::GetCurrentTexturePalette(UINT* palette_number) noexcept
{
   return _device->GetCurrentTexturePalette(palette_number);
}

HRESULT Device::SetScissorRect(const RECT* rect) noexcept
{
   return _device->SetScissorRect(rect);
}

HRESULT Device::GetScissorRect(RECT* rect) noexcept
{
   return _device->GetScissorRect(rect);
}

HRESULT Device::SetSoftwareVertexProcessing(BOOL software) noexcept
{
   return _device->SetSoftwareVertexProcessing(software);
}

BOOL Device::GetSoftwareVertexProcessing() noexcept
{
   return _device->GetSoftwareVertexProcessing();
}

HRESULT Device::SetNPatchMode(float segments) noexcept
{
   return _device->SetNPatchMode(segments);
}

float Device::GetNPatchMode() noexcept
{
   return _device->GetNPatchMode();
}

HRESULT Device::DrawPrimitive(D3DPRIMITIVETYPE primitive_type,
                              UINT start_vertex, UINT primitive_count) noexcept
{
   return _device->DrawPrimitive(primitive_type, start_vertex, primitive_count);
}

HRESULT Device::DrawIndexedPrimitive(D3DPRIMITIVETYPE primitive_type,
                                     INT base_vertex_index,
                                     UINT min_vertex_index, UINT num_vertices,
                                     UINT start_Index, UINT prim_Count) noexcept
{
   return _device->DrawIndexedPrimitive(primitive_type, base_vertex_index,
                                        min_vertex_index, num_vertices,
                                        start_Index, prim_Count);
}

HRESULT Device::DrawPrimitiveUP(D3DPRIMITIVETYPE primitive_type, UINT primitive_count,
                                const void* vertexstreamzerodata,
                                UINT vertexstreamzerostride) noexcept
{
   return _device->DrawPrimitiveUP(primitive_type, primitive_count,
                                   vertexstreamzerodata, vertexstreamzerostride);
}

HRESULT Device::DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE primitive_type,
                                       UINT min_vertex_index, UINT num_vertices,
                                       UINT primitive_count, const void* index_data,
                                       D3DFORMAT index_data_format,
                                       const void* vertex_stream_zero_data,
                                       UINT vertex_stream_zero_stride) noexcept
{
   return _device->DrawIndexedPrimitiveUP(primitive_type, min_vertex_index,
                                          num_vertices, primitive_count, index_data,
                                          index_data_format, vertex_stream_zero_data,
                                          vertex_stream_zero_stride);
}

HRESULT Device::ProcessVertices(UINT src_start_index, UINT dest_index,
                                UINT vertex_count, IDirect3DVertexBuffer9* dest_buffer,
                                IDirect3DVertexDeclaration9* vertex_decl,
                                DWORD flags) noexcept
{
   return _device->ProcessVertices(src_start_index, dest_index, vertex_count,
                                   dest_buffer, vertex_decl, flags);
}

HRESULT Device::CreateVertexDeclaration(const D3DVERTEXELEMENT9* vertex_elements,
                                        IDirect3DVertexDeclaration9** decl) noexcept
{
   return _device->CreateVertexDeclaration(vertex_elements, decl);
}

HRESULT Device::SetVertexDeclaration(IDirect3DVertexDeclaration9* decl) noexcept
{
   return _device->SetVertexDeclaration(decl);
}

HRESULT Device::GetVertexDeclaration(IDirect3DVertexDeclaration9** decl) noexcept
{
   return _device->GetVertexDeclaration(decl);
}

HRESULT Device::SetFVF(DWORD fvf) noexcept
{
   return _device->SetFVF(fvf);
}

HRESULT Device::GetFVF(DWORD* fvf) noexcept
{
   return _device->GetFVF(fvf);
}

HRESULT Device::GetVertexShader(IDirect3DVertexShader9** shader) noexcept
{
   return _device->GetVertexShader(shader);
}

HRESULT Device::GetVertexShaderConstantF(UINT start_register, float* constant_data,
                                         UINT vector4f_count) noexcept
{
   return _device->GetVertexShaderConstantF(start_register, constant_data,
                                            vector4f_count);
}

HRESULT Device::SetVertexShaderConstantI(UINT start_register, const int* constant_data,
                                         UINT vector4i_count) noexcept
{
   return _device->SetVertexShaderConstantI(start_register, constant_data,
                                            vector4i_count);
}

HRESULT Device::GetVertexShaderConstantI(UINT start_register, int* constant_data,
                                         UINT vector4i_count) noexcept
{
   return _device->GetVertexShaderConstantI(start_register, constant_data,
                                            vector4i_count);
}

HRESULT Device::SetVertexShaderConstantB(UINT start_register, const BOOL* constant_data,
                                         UINT bool_count) noexcept
{
   return _device->SetVertexShaderConstantB(start_register, constant_data, bool_count);
}

HRESULT Device::GetVertexShaderConstantB(UINT start_register, BOOL* constant_data,
                                         UINT bool_count) noexcept
{
   return _device->GetVertexShaderConstantB(start_register, constant_data, bool_count);
}

HRESULT Device::SetStreamSource(UINT stream_number, IDirect3DVertexBuffer9* stream_data,
                                UINT offset_in_bytes, UINT stride) noexcept
{
   return _device->SetStreamSource(stream_number, stream_data, offset_in_bytes, stride);
}

HRESULT Device::GetStreamSource(UINT stream_number, IDirect3DVertexBuffer9** stream_data,
                                UINT* offset_in_bytes, UINT* stride) noexcept
{
   return _device->GetStreamSource(stream_number, stream_data, offset_in_bytes, stride);
}

HRESULT Device::SetStreamSourceFreq(UINT stream_number, UINT setting) noexcept
{
   return _device->SetStreamSourceFreq(stream_number, setting);
}

HRESULT Device::GetStreamSourceFreq(UINT stream_number, UINT* setting) noexcept
{
   return _device->GetStreamSourceFreq(stream_number, setting);
}

HRESULT Device::SetIndices(IDirect3DIndexBuffer9* index_data) noexcept
{
   return _device->SetIndices(index_data);
}

HRESULT Device::GetIndices(IDirect3DIndexBuffer9** index_data) noexcept
{
   return _device->GetIndices(index_data);
}

HRESULT Device::GetPixelShader(IDirect3DPixelShader9** shader) noexcept
{
   return _device->GetPixelShader(shader);
}

HRESULT Device::SetPixelShaderConstantF(UINT start_register, const float* constant_data,
                                        UINT vector4f_count) noexcept
{
   return _device->SetPixelShaderConstantF(start_register, constant_data, vector4f_count);
}

HRESULT Device::GetPixelShaderConstantF(UINT Start_Register, float* constant_data,
                                        UINT vector4f_count) noexcept
{
   return _device->GetPixelShaderConstantF(Start_Register, constant_data, vector4f_count);
}

HRESULT Device::SetPixelShaderConstantI(UINT start_register, const int* constant_data,
                                        UINT vector4i_count) noexcept
{
   return _device->SetPixelShaderConstantI(start_register, constant_data, vector4i_count);
}

HRESULT Device::GetPixelShaderConstantI(UINT start_register, int* constant_data,
                                        UINT vector4i_count) noexcept
{
   return _device->GetPixelShaderConstantI(start_register, constant_data, vector4i_count);
}

HRESULT Device::SetPixelShaderConstantB(UINT start_register, const BOOL* constant_data,
                                        UINT bool_count) noexcept
{
   return _device->SetPixelShaderConstantB(start_register, constant_data, bool_count);
}

HRESULT Device::GetPixelShaderConstantB(UINT start_register, BOOL* constant_data,
                                        UINT bool_count) noexcept
{
   return _device->GetPixelShaderConstantB(start_register, constant_data, bool_count);
}

HRESULT Device::DrawRectPatch(UINT handle, const float* num_segs,
                              const D3DRECTPATCH_INFO* rect_patch_info) noexcept
{
   return _device->DrawRectPatch(handle, num_segs, rect_patch_info);
}

HRESULT Device::DrawTriPatch(UINT handle, const float* num_segs,
                             const D3DTRIPATCH_INFO* tri_patch_info) noexcept
{
   return _device->DrawTriPatch(handle, num_segs, tri_patch_info);
}

HRESULT Device::DeletePatch(UINT handle) noexcept
{
   return _device->DeletePatch(handle);
}

HRESULT Device::CreateQuery(D3DQUERYTYPE type, IDirect3DQuery9** query) noexcept
{
   return _device->CreateQuery(type, query);
}
}
