#pragma once

#include "com_ptr.hpp"

#include <atomic>
#include <optional>
#include <utility>

#include <d3d9.h>

namespace sp::direct3d {

constexpr IID rendertarget_uuid = {0xefbdac24,
                                   0x26e0,
                                   0x4d41,
                                   {0x8d, 0x6d, 0x71, 0x2, 0xba, 0x2d, 0x8a, 0xf4}};

class Rendertarget : public IDirect3DTexture9 {
public:
   static auto create(IDirect3DDevice9& device, UINT width, UINT height,
                      D3DFORMAT format) noexcept
      -> std::pair<HRESULT, Com_ptr<Rendertarget>>;

   IDirect3DTexture9& texture() noexcept;

   const IDirect3DTexture9& texture() const noexcept;

   IDirect3DSurface9& surface() noexcept;

   const IDirect3DSurface9& surface() const noexcept;

   HRESULT __stdcall QueryInterface(const IID& iid, void** object) noexcept override;
   ULONG __stdcall AddRef() noexcept override;
   ULONG __stdcall Release() noexcept override;

   HRESULT __stdcall GetDevice(IDirect3DDevice9** device) noexcept override;

   HRESULT __stdcall SetPrivateData(const GUID& guid, const void* data,
                                    DWORD data_size, DWORD flags) noexcept override;
   HRESULT __stdcall GetPrivateData(const GUID& guid, void* data,
                                    DWORD* data_size) noexcept override;
   HRESULT __stdcall FreePrivateData(const GUID& guid) noexcept override;

   DWORD __stdcall SetPriority(DWORD priority) noexcept override;
   DWORD __stdcall GetPriority() noexcept override;

   void __stdcall PreLoad() noexcept override;

   D3DRESOURCETYPE __stdcall GetType() noexcept override;

   DWORD __stdcall SetLOD(DWORD lod) noexcept override;
   DWORD __stdcall GetLOD() noexcept override;

   DWORD __stdcall GetLevelCount() noexcept override;

   HRESULT __stdcall SetAutoGenFilterType(D3DTEXTUREFILTERTYPE filter_type) noexcept override;
   D3DTEXTUREFILTERTYPE __stdcall GetAutoGenFilterType() noexcept override;

   void __stdcall GenerateMipSubLevels() noexcept override;

   HRESULT __stdcall GetLevelDesc(UINT level, D3DSURFACE_DESC* desc) noexcept override;

   HRESULT __stdcall GetSurfaceLevel(UINT level,
                                     IDirect3DSurface9** surface) noexcept override;

   HRESULT __stdcall LockRect(UINT level, D3DLOCKED_RECT* locked_rect,
                              const RECT* rect, DWORD flags) noexcept override;
   HRESULT __stdcall UnlockRect(UINT level) noexcept override;

   HRESULT __stdcall AddDirtyRect(const RECT* dirty_rect) noexcept override;

   // HRESULT __stdcall GetContainer(const IID& iid, void** container) noexcept override;
   //
   // HRESULT __stdcall GetDesc(D3DSURFACE_DESC* desc) noexcept override;
   //
   // HRESULT __stdcall LockRect(D3DLOCKED_RECT* locked_rect, const RECT* rect,
   //                            DWORD flags) noexcept override;
   //
   // HRESULT __stdcall UnlockRect() noexcept override;
   //
   // HRESULT __stdcall GetDC(HDC* hdc) noexcept override;
   //
   // HRESULT __stdcall ReleaseDC(HDC hdc) noexcept override;

private:
   Rendertarget(Com_ptr<IDirect3DTexture9> texture,
                Com_ptr<IDirect3DSurface9> surface, UINT width, UINT height);

   ~Rendertarget() = default;

   const Com_ptr<IDirect3DTexture9> _texture;
   const Com_ptr<IDirect3DSurface9> _surface;

   const UINT _width;
   const UINT _height;

   Com_ptr<IDirect3DTexture9> _depth_texture;

   std::atomic<ULONG> _ref_count{1};
};
}
