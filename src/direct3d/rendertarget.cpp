
#include "rendertarget.hpp"

namespace sp::direct3d {

auto Rendertarget::create(IDirect3DDevice9& device, UINT width, UINT height,
                          D3DFORMAT format) noexcept
   -> std::pair<HRESULT, Com_ptr<Rendertarget>>
{
   Com_ptr<IDirect3DTexture9> texture;

   auto result =
      device.CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, format,
                           D3DPOOL_DEFAULT, texture.clear_and_assign(), nullptr);

   if (result != S_OK) return {result, nullptr};

   Com_ptr<IDirect3DSurface9> surface;

   result = texture->GetSurfaceLevel(0, surface.clear_and_assign());

   if (result != S_OK) return {result, nullptr};

   Com_ptr<Rendertarget> render_target{
      new Rendertarget{std::move(texture), std::move(surface), width, height}};

   return {S_OK, std::move(render_target)};
}

Rendertarget::Rendertarget(Com_ptr<IDirect3DTexture9> texture,
                           Com_ptr<IDirect3DSurface9> surface, UINT width, UINT height)
   : _texture{std::move(texture)}, _surface{std::move(surface)}, _width{width},
     _height{height} {};

IDirect3DTexture9& Rendertarget::texture() noexcept
{
   return *_texture;
}

const IDirect3DTexture9& Rendertarget::texture() const noexcept
{
   return *_texture;
}

IDirect3DSurface9& Rendertarget::surface() noexcept
{
   return *_surface;
}

const IDirect3DSurface9& Rendertarget::surface() const noexcept
{
   return *_surface;
}

HRESULT Rendertarget::QueryInterface(const IID& iid, void** object) noexcept
{
   if (object == nullptr) return E_POINTER;

   bool success = false;

   if (iid == rendertarget_uuid) {
      *object = this;
      success = true;
   }
   else if (iid == IID_IDirect3DTexture9) {
      *object = static_cast<IDirect3DTexture9*>(this);
      success = true;
   }
   // else if (iid == IID_IDirect3DSurface9) {
   //   *object = static_cast<IDirect3DSurface9*>(this);
   //   success = true;
   //}
   else if (iid == IID_IDirect3DBaseTexture9) {
      *object = static_cast<IDirect3DBaseTexture9*>(this);
      success = true;
   }
   else if (iid == IID_IDirect3DResource9) {
      *object =
         static_cast<IDirect3DResource9*>(static_cast<IDirect3DTexture9*>(this));
      success = true;
   }
   else if (iid == IID_IUnknown) {
      *object = static_cast<IUnknown*>(static_cast<IDirect3DTexture9*>(this));
      success = true;
   }

   if (success) {
      this->AddRef();

      return S_OK;
   }

   return E_NOINTERFACE;
}

ULONG Rendertarget::AddRef() noexcept
{
   return _ref_count.fetch_add(1);
}

ULONG Rendertarget::Release() noexcept
{
   const auto ref_count = _ref_count.fetch_sub(1);

   if (ref_count == 0) delete this;

   return ref_count;
}

HRESULT Rendertarget::GetDevice(IDirect3DDevice9** device) noexcept
{
   return _texture->GetDevice(device);
}

HRESULT Rendertarget::SetPrivateData(const GUID& guid, const void* data,
                                     DWORD data_size, DWORD flags) noexcept
{
   return _texture->SetPrivateData(guid, data, data_size, flags);
}

HRESULT Rendertarget::GetPrivateData(const GUID& guid, void* data, DWORD* data_size) noexcept
{
   return _texture->GetPrivateData(guid, data, data_size);
}

HRESULT Rendertarget::FreePrivateData(const GUID& guid) noexcept
{
   return _texture->FreePrivateData(guid);
}

DWORD Rendertarget::SetPriority(DWORD priority) noexcept
{
   return _texture->SetPriority(priority);
}

DWORD Rendertarget::GetPriority() noexcept
{
   return _texture->GetPriority();
}

void Rendertarget::PreLoad() noexcept
{
   return _texture->PreLoad();
}

D3DRESOURCETYPE Rendertarget::GetType() noexcept
{
   return _texture->GetType();
}

DWORD Rendertarget::SetLOD(DWORD lod) noexcept
{
   return _texture->SetLOD(lod);
}

DWORD Rendertarget::GetLOD() noexcept
{
   return _texture->GetLOD();
}

DWORD Rendertarget::GetLevelCount() noexcept
{
   return _texture->GetLevelCount();
}

HRESULT Rendertarget::SetAutoGenFilterType(D3DTEXTUREFILTERTYPE filter_type) noexcept
{
   return _texture->SetAutoGenFilterType(filter_type);
}

D3DTEXTUREFILTERTYPE Rendertarget::GetAutoGenFilterType() noexcept
{
   return _texture->GetAutoGenFilterType();
}

void Rendertarget::GenerateMipSubLevels() noexcept
{
   return _texture->GenerateMipSubLevels();
}

HRESULT Rendertarget::GetLevelDesc(UINT level, D3DSURFACE_DESC* desc) noexcept
{
   return _texture->GetLevelDesc(level, desc);
}

HRESULT Rendertarget::GetSurfaceLevel(UINT level, IDirect3DSurface9** surface) noexcept
{
   return _texture->GetSurfaceLevel(level, surface);
}

HRESULT Rendertarget::LockRect(UINT level, D3DLOCKED_RECT* locked_rect,
                               const RECT* rect, DWORD flags) noexcept
{
   return _texture->LockRect(level, locked_rect, rect, flags);
}

HRESULT Rendertarget::UnlockRect(UINT level) noexcept
{
   return _texture->UnlockRect(level);
}

HRESULT Rendertarget::AddDirtyRect(const RECT* dirty_rect) noexcept
{
   return _texture->AddDirtyRect(dirty_rect);
}

// HRESULT Rendertarget::GetContainer(const IID& iid, void** container) noexcept
//{
//   return _surface->GetContainer(iid, container);
//}
//
// HRESULT Rendertarget::GetDesc(D3DSURFACE_DESC* desc) noexcept
//{
//   return _surface->GetDesc(desc);
//}
//
// HRESULT Rendertarget::LockRect(D3DLOCKED_RECT* locked_rect, const RECT* rect,
//                               DWORD flags) noexcept
//{
//   return _surface->LockRect(locked_rect, rect, flags);
//}
//
// HRESULT Rendertarget::UnlockRect() noexcept
//{
//   return _surface->UnlockRect();
//}
//
// HRESULT Rendertarget::GetDC(HDC* hdc) noexcept
//{
//   return _surface->GetDC(hdc);
//}
//
// HRESULT Rendertarget::ReleaseDC(HDC hdc) noexcept
//{
//   return _surface->ReleaseDC(hdc);
//}
}
