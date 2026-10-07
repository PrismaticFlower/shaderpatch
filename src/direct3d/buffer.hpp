#pragma once

#include "../core/shader_patch.hpp"
#include "../logger.hpp"
#include "debug_trace.hpp"
#include "resource.hpp"
#include "upload_scratch_buffer.hpp"

#include <type_traits>
#include <utility>

#include <d3d9.h>

namespace sp::d3d9 {

enum class Buffer_type { vertex_buffer, index_buffer };

template<Buffer_type type>
class Basic_buffer_managed : public Resource {
public:
   static Com_ptr<Basic_buffer_managed> create(core::Shader_patch& shader_patch,
                                               const UINT size) noexcept
   {
      return Com_ptr{new Basic_buffer_managed{shader_patch, size}};
   }

   static_assert(type == Buffer_type::vertex_buffer || type == Buffer_type::index_buffer);

   using Description_type =
      std::conditional_t<type == Buffer_type::vertex_buffer, D3DVERTEXBUFFER_DESC, D3DINDEXBUFFER_DESC>;

   Basic_buffer_managed(const Basic_buffer_managed&) = delete;
   Basic_buffer_managed& operator=(const Basic_buffer_managed&) = delete;

   Basic_buffer_managed(Basic_buffer_managed&&) = delete;
   Basic_buffer_managed& operator=(Basic_buffer_managed&&) = delete;

   HRESULT __stdcall QueryInterface(const IID& iid, void** object) noexcept override
   {
      Debug_trace::func(__FUNCSIG__);

      if (!object) return E_INVALIDARG;

      const auto self_iid = type == Buffer_type::vertex_buffer
                               ? IID_IDirect3DVertexBuffer9
                               : IID_IDirect3DIndexBuffer9;

      if (iid == IID_IUnknown) {
         *object = static_cast<IUnknown*>(this);
      }
      else if (iid == IID_IDirect3DResource9) {
         *object = static_cast<Resource*>(this);
      }
      else if (iid == self_iid) {
         *object = this;
      }
      else {
         *object = nullptr;

         return E_NOINTERFACE;
      }

      AddRef();

      return S_OK;
   }

   ULONG __stdcall AddRef() noexcept override
   {
      Debug_trace::func(__FUNCSIG__);

      return _ref_count += 1;
   }

   ULONG __stdcall Release() noexcept override
   {
      Debug_trace::func(__FUNCSIG__);

      const auto ref_count = _ref_count -= 1;

      if (ref_count == 0) delete this;

      return ref_count;
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetDevice(IDirect3DDevice9**) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall SetPrivateData(const GUID&,
                                                                    const void*, DWORD,
                                                                    DWORD) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetPrivateData(const GUID&,
                                                                    void*, DWORD*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall FreePrivateData(const GUID&) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] DWORD __stdcall SetPriority(DWORD) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] DWORD __stdcall GetPriority() noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] void __stdcall PreLoad() noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   D3DRESOURCETYPE __stdcall GetType() noexcept override
   {
      Debug_trace::func(__FUNCSIG__);

      return type == Buffer_type::vertex_buffer ? D3DRTYPE_VERTEXBUFFER
                                                : D3DRTYPE_INDEXBUFFER;
   }

   virtual HRESULT __stdcall Lock(UINT lock_offset, UINT lock_size, void** data,
                                  DWORD flags) noexcept
   {
      Debug_trace::func(__FUNCSIG__);

      if (lock_offset == 0 && lock_size == 0) {
         lock_size = _size;
      }

      if (!data) return D3DERR_INVALIDCALL;
      if (lock_size > _size) return D3DERR_INVALIDCALL;
      if (lock_offset > (_size - lock_size)) return D3DERR_INVALIDCALL;

      if (flags != D3DLOCK_NOSYSLOCK) {
         log_and_terminate("Unexpected buffer lock flags!");
      }

      if (std::exchange(_lock_status, true)) {
         log_and_terminate("Unexpected buffer lock!");
      }

      _lock_offset = lock_offset;
      _lock_size = lock_size;
      _lock_data = upload_scratch_buffer.lock(_size - lock_offset);

      *data = _lock_data;

      return S_OK;
   }

   virtual HRESULT __stdcall Unlock() noexcept
   {
      Debug_trace::func(__FUNCSIG__);

      if (!std::exchange(_lock_status, false)) {
         log_and_terminate("Unexpected buffer unlock!");
      }

      _shader_patch.update_ia_buffer(*_buffer, std::exchange(_lock_offset, 0),
                                     std::exchange(_lock_size, 0),
                                     std::exchange(_lock_data, nullptr));

      upload_scratch_buffer.unlock();

      return S_OK;
   }

   [[deprecated("unimplemented")]] virtual HRESULT __stdcall GetDesc(Description_type*) noexcept
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

private:
   Basic_buffer_managed(core::Shader_patch& shader_patch, const UINT size) noexcept
      : _shader_patch{shader_patch}, _size{size}
   {
      if (type == Buffer_type::index_buffer) {
         this->resource = Index_buffer_view{.buffer = _buffer.get(),
                                            .format = DXGI_FORMAT_R16_UINT};
      }
      else {
         this->resource = _buffer.get();
      }
   }

   ~Basic_buffer_managed() = default;

   core::Shader_patch& _shader_patch;
   const UINT _size;
   const Com_ptr<ID3D11Buffer> _buffer{
      _shader_patch.create_ia_buffer(_size, type == Buffer_type::vertex_buffer,
                                     type == Buffer_type::index_buffer, false)};

   UINT _lock_offset{};
   UINT _lock_size{};
   std::byte* _lock_data = nullptr;
   bool _lock_status = false;

   ULONG _ref_count = 1;
};

using Vertex_buffer_managed = Basic_buffer_managed<Buffer_type::vertex_buffer>;
using Index_buffer_16_managed = Basic_buffer_managed<Buffer_type::index_buffer>;

static_assert(
   !std::has_virtual_destructor_v<Vertex_buffer_managed>,
   "Vertex_buffer_managed must not have a virtual destructor as it will cause "
   "an ABI break.");

template<Buffer_type type>
class Basic_buffer_dynamic : public Resource {
public:
   static Com_ptr<Basic_buffer_dynamic> create(core::Shader_patch& shader_patch,
                                               const UINT size) noexcept
   {
      return Com_ptr{new Basic_buffer_dynamic{shader_patch, size}};
   }

   static_assert(type == Buffer_type::vertex_buffer || type == Buffer_type::index_buffer);

   using Description_type =
      std::conditional_t<type == Buffer_type::vertex_buffer, D3DVERTEXBUFFER_DESC, D3DINDEXBUFFER_DESC>;

   Basic_buffer_dynamic(const Basic_buffer_dynamic&) = delete;
   Basic_buffer_dynamic& operator=(const Basic_buffer_dynamic&) = delete;

   Basic_buffer_dynamic(Basic_buffer_dynamic&&) = delete;
   Basic_buffer_dynamic& operator=(Basic_buffer_dynamic&&) = delete;

   HRESULT __stdcall QueryInterface(const IID& iid, void** object) noexcept override
   {
      Debug_trace::func(__FUNCSIG__);

      if (!object) return E_INVALIDARG;

      const auto self_iid = type == Buffer_type::vertex_buffer
                               ? IID_IDirect3DVertexBuffer9
                               : IID_IDirect3DIndexBuffer9;

      if (iid == IID_IUnknown) {
         *object = static_cast<IUnknown*>(this);
      }
      else if (iid == IID_IDirect3DResource9) {
         *object = static_cast<Resource*>(this);
      }
      else if (iid == self_iid) {
         *object = this;
      }
      else {
         *object = nullptr;

         return E_NOINTERFACE;
      }

      AddRef();

      return S_OK;
   }

   ULONG __stdcall AddRef() noexcept override
   {
      Debug_trace::func(__FUNCSIG__);

      return _ref_count += 1;
   }

   ULONG __stdcall Release() noexcept override
   {
      Debug_trace::func(__FUNCSIG__);

      const auto ref_count = _ref_count -= 1;

      if (ref_count == 0) delete this;

      return ref_count;
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetDevice(IDirect3DDevice9**) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall SetPrivateData(const GUID&,
                                                                    const void*, DWORD,
                                                                    DWORD) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetPrivateData(const GUID&,
                                                                    void*, DWORD*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall FreePrivateData(const GUID&) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] DWORD __stdcall SetPriority(DWORD) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] DWORD __stdcall GetPriority() noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] void __stdcall PreLoad() noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   D3DRESOURCETYPE __stdcall GetType() noexcept override
   {
      Debug_trace::func(__FUNCSIG__);

      return type == Buffer_type::vertex_buffer ? D3DRTYPE_VERTEXBUFFER
                                                : D3DRTYPE_INDEXBUFFER;
   }

   virtual HRESULT __stdcall Lock(UINT lock_offset, UINT lock_size, void** data,
                                  DWORD flags) noexcept
   {
      Debug_trace::func(__FUNCSIG__);

      if (lock_offset == 0 && lock_size == 0) {
         lock_size = _size;
      }

      if (!data) return D3DERR_INVALIDCALL;
      if (lock_size > _size) return D3DERR_INVALIDCALL;
      if (lock_offset > (_size - lock_size)) return D3DERR_INVALIDCALL;

      if (_lock_count == 0) {
         _mapped_data =
            _shader_patch.map_ia_buffer(*_buffer, lock_flags_to_map_type(flags));

         if (!_mapped_data) return D3DERR_INVALIDCALL;
      }

      _lock_count += 1;

      *data = _mapped_data + lock_offset;

      return S_OK;
   }

   virtual HRESULT __stdcall Unlock() noexcept
   {
      Debug_trace::func(__FUNCSIG__);

      if (--_lock_count == 0 && std::exchange(_mapped_data, nullptr)) {
         _shader_patch.unmap_ia_buffer(*_buffer);
      }
      else if (_lock_count == -1) {
         log_and_terminate("Unexpected buffer unlock!");
      }

      return S_OK;
   }

   [[deprecated("unimplemented")]] virtual HRESULT __stdcall GetDesc(Description_type*) noexcept
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

private:
   Basic_buffer_dynamic(core::Shader_patch& shader_patch, const UINT size) noexcept
      : _shader_patch{shader_patch}, _size{size}
   {
      if (type == Buffer_type::index_buffer) {
         this->resource = Index_buffer_view{.buffer = _buffer.get(),
                                            .format = DXGI_FORMAT_R16_UINT};
      }
      else {
         this->resource = _buffer.get();
      }
   }

   ~Basic_buffer_dynamic() = default;

   D3D11_MAP lock_flags_to_map_type(const DWORD flags) const noexcept
   {
      if (flags & D3DLOCK_READONLY) return D3D11_MAP_READ;
      if (flags & D3DLOCK_DISCARD) return D3D11_MAP_WRITE_DISCARD;
      if (flags & D3DLOCK_NOOVERWRITE) return D3D11_MAP_WRITE_NO_OVERWRITE;

      return D3D11_MAP_WRITE;
   }

   core::Shader_patch& _shader_patch;
   const UINT _size;
   const Com_ptr<ID3D11Buffer> _buffer{
      _shader_patch.create_ia_buffer(_size, type == Buffer_type::vertex_buffer,
                                     type == Buffer_type::index_buffer, true)};

   int _lock_count = 0;
   std::byte* _mapped_data = nullptr;
   ULONG _ref_count = 1;
};

using Vertex_buffer_dynamic = Basic_buffer_dynamic<Buffer_type::vertex_buffer>;
using Index_buffer_16_dynamic = Basic_buffer_dynamic<Buffer_type::index_buffer>;

static_assert(
   !std::has_virtual_destructor_v<Vertex_buffer_dynamic>,
   "Vertex_buffer_dynamic must not have a virtual destructor as it will cause "
   "an ABI break.");

class Index_buffer_16_to_32_managed : public Resource {
public:
   static Com_ptr<Index_buffer_16_to_32_managed> create(core::Shader_patch& shader_patch,
                                                        const UINT size) noexcept
   {
      return Com_ptr{new Index_buffer_16_to_32_managed{shader_patch, size}};
   }

   Index_buffer_16_to_32_managed(const Index_buffer_16_to_32_managed&) = delete;
   Index_buffer_16_to_32_managed& operator=(const Index_buffer_16_to_32_managed&) = delete;

   Index_buffer_16_to_32_managed(Index_buffer_16_to_32_managed&&) = delete;
   Index_buffer_16_to_32_managed& operator=(Index_buffer_16_to_32_managed&&) = delete;

   HRESULT __stdcall QueryInterface(const IID& iid, void** object) noexcept override
   {
      Debug_trace::func(__FUNCSIG__);

      if (!object) return E_INVALIDARG;

      const auto self_iid = IID_IDirect3DIndexBuffer9;

      if (iid == IID_IUnknown) {
         *object = static_cast<IUnknown*>(this);
      }
      else if (iid == IID_IDirect3DResource9) {
         *object = static_cast<Resource*>(this);
      }
      else if (iid == self_iid) {
         *object = this;
      }
      else {
         *object = nullptr;

         return E_NOINTERFACE;
      }

      AddRef();

      return S_OK;
   }

   ULONG __stdcall AddRef() noexcept override
   {
      Debug_trace::func(__FUNCSIG__);

      return _ref_count += 1;
   }

   ULONG __stdcall Release() noexcept override
   {
      Debug_trace::func(__FUNCSIG__);

      const auto ref_count = _ref_count -= 1;

      if (ref_count == 0) delete this;

      return ref_count;
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetDevice(IDirect3DDevice9**) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall SetPrivateData(const GUID&,
                                                                    const void*, DWORD,
                                                                    DWORD) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetPrivateData(const GUID&,
                                                                    void*, DWORD*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall FreePrivateData(const GUID&) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] DWORD __stdcall SetPriority(DWORD) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] DWORD __stdcall GetPriority() noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] void __stdcall PreLoad() noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   D3DRESOURCETYPE __stdcall GetType() noexcept override
   {
      Debug_trace::func(__FUNCSIG__);

      return D3DRTYPE_INDEXBUFFER;
   }

   virtual HRESULT __stdcall Lock(UINT lock_offset, UINT lock_size, void** data,
                                  DWORD flags) noexcept
   {
      Debug_trace::func(__FUNCSIG__);

      if (lock_offset == 0 && lock_size == 0) {
         lock_size = _size;
      }

      if (!data) return D3DERR_INVALIDCALL;
      if (lock_size > _size) return D3DERR_INVALIDCALL;
      if (lock_offset > (_size - lock_size)) return D3DERR_INVALIDCALL;

      if (flags != D3DLOCK_NOSYSLOCK) {
         log_and_terminate("Unexpected buffer lock flags!");
      }

      if (std::exchange(_lock_status, true)) {
         log_and_terminate("Unexpected buffer lock!");
      }

      _lock_offset = lock_offset;
      _lock_size = lock_size;
      _lock_data = upload_scratch_buffer.lock((_size - lock_offset) * 2);

      *data = _lock_data;

      return S_OK;
   }

   virtual HRESULT __stdcall Unlock() noexcept
   {
      Debug_trace::func(__FUNCSIG__);

      if (!std::exchange(_lock_status, false)) {
         log_and_terminate("Unexpected buffer unlock!");
      }

      const std::ptrdiff_t count = (std::ptrdiff_t)((_lock_size + 1u) / 2u);

      std::uint16_t* u16 = reinterpret_cast<std::uint16_t*>(_lock_data);
      std::uint32_t* u32 = reinterpret_cast<std::uint32_t*>(_lock_data);

      for (std::ptrdiff_t i = count - 1; i >= 0; --i) {
         std::uint32_t index = u16[i];

         std::memcpy(&u32[i], &index, sizeof(index));
      }

      _shader_patch.update_ia_buffer(*_buffer, std::exchange(_lock_offset, 0) * 2,
                                     std::exchange(_lock_size, 0) * 2,
                                     reinterpret_cast<const std::byte*>(u32));

      _lock_data = nullptr;
      upload_scratch_buffer.unlock();

      return S_OK;
   }

   [[deprecated("unimplemented")]] virtual HRESULT __stdcall GetDesc(D3DINDEXBUFFER_DESC*) noexcept
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

private:
   Index_buffer_16_to_32_managed(core::Shader_patch& shader_patch, const UINT size) noexcept
      : _shader_patch{shader_patch}, _size{size}
   {
      this->resource =
         Index_buffer_view{.buffer = _buffer.get(), .format = DXGI_FORMAT_R32_UINT};
   }

   ~Index_buffer_16_to_32_managed() = default;

   core::Shader_patch& _shader_patch;
   const UINT _size;
   const Com_ptr<ID3D11Buffer> _buffer{
      _shader_patch.create_ia_buffer(_size * 2, false, true, false)};

   UINT _lock_offset{};
   UINT _lock_size{};
   std::byte* _lock_data = nullptr;
   bool _lock_status = false;

   ULONG _ref_count = 1;
};

class Index_buffer_16_to_32_dynamic : public Resource {
public:
   static Com_ptr<Index_buffer_16_to_32_dynamic> create(core::Shader_patch& shader_patch,
                                                        const UINT size) noexcept
   {
      return Com_ptr{new Index_buffer_16_to_32_dynamic{shader_patch, size}};
   }

   Index_buffer_16_to_32_dynamic(const Index_buffer_16_to_32_dynamic&) = delete;
   Index_buffer_16_to_32_dynamic& operator=(const Index_buffer_16_to_32_dynamic&) = delete;

   Index_buffer_16_to_32_dynamic(Index_buffer_16_to_32_dynamic&&) = delete;
   Index_buffer_16_to_32_dynamic& operator=(Index_buffer_16_to_32_dynamic&&) = delete;

   HRESULT __stdcall QueryInterface(const IID& iid, void** object) noexcept override
   {
      Debug_trace::func(__FUNCSIG__);

      if (!object) return E_INVALIDARG;

      const auto self_iid = IID_IDirect3DIndexBuffer9;

      if (iid == IID_IUnknown) {
         *object = static_cast<IUnknown*>(this);
      }
      else if (iid == IID_IDirect3DResource9) {
         *object = static_cast<Resource*>(this);
      }
      else if (iid == self_iid) {
         *object = this;
      }
      else {
         *object = nullptr;

         return E_NOINTERFACE;
      }

      AddRef();

      return S_OK;
   }

   ULONG __stdcall AddRef() noexcept override
   {
      Debug_trace::func(__FUNCSIG__);

      return _ref_count += 1;
   }

   ULONG __stdcall Release() noexcept override
   {
      Debug_trace::func(__FUNCSIG__);

      const auto ref_count = _ref_count -= 1;

      if (ref_count == 0) delete this;

      return ref_count;
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetDevice(IDirect3DDevice9**) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall SetPrivateData(const GUID&,
                                                                    const void*, DWORD,
                                                                    DWORD) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetPrivateData(const GUID&,
                                                                    void*, DWORD*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall FreePrivateData(const GUID&) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] DWORD __stdcall SetPriority(DWORD) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] DWORD __stdcall GetPriority() noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] void __stdcall PreLoad() noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   D3DRESOURCETYPE __stdcall GetType() noexcept override
   {
      Debug_trace::func(__FUNCSIG__);

      return D3DRTYPE_INDEXBUFFER;
   }

   virtual HRESULT __stdcall Lock(UINT lock_offset, UINT lock_size, void** data,
                                  DWORD flags) noexcept
   {
      Debug_trace::func(__FUNCSIG__);

      if (lock_offset == 0 && lock_size == 0) {
         lock_size = _size;
      }

      if (!data) return D3DERR_INVALIDCALL;
      if (lock_size > _size) return D3DERR_INVALIDCALL;
      if (lock_offset > (_size - lock_size)) return D3DERR_INVALIDCALL;

      _lock_flags = flags;
      _locked_start = std::min(_locked_start, lock_offset);
      _locked_end = std::max(_locked_end, lock_offset + lock_size);

      _lock_count += 1;

      *data = _cpu_data.data() + lock_offset;

      return S_OK;
   }

   virtual HRESULT __stdcall Unlock() noexcept
   {
      Debug_trace::func(__FUNCSIG__);

      if (--_lock_count == 0) {
         const std::ptrdiff_t count = (_size / 2);

         std::byte* mapped_data =
            _shader_patch.map_ia_buffer(*_buffer, lock_flags_to_map_type(_lock_flags));

         std::uint16_t* u16 = reinterpret_cast<std::uint16_t*>(_cpu_data.data());
         std::uint32_t* u32 = reinterpret_cast<std::uint32_t*>(mapped_data);

         for (std::ptrdiff_t i = 0; i < count; ++i) {
            std::uint32_t index = u16[i];

            std::memcpy(&u32[i], &index, sizeof(index));
         }

         _shader_patch.unmap_ia_buffer(*_buffer);

         _locked_start = UINT_MAX;
         _locked_end = 0;
      }
      else if (_lock_count == -1) {
         log_and_terminate("Unexpected buffer unlock!");
      }

      return S_OK;
   }

   [[deprecated("unimplemented")]] virtual HRESULT __stdcall GetDesc(D3DINDEXBUFFER_DESC*) noexcept
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

private:
   Index_buffer_16_to_32_dynamic(core::Shader_patch& shader_patch, const UINT size) noexcept
      : _shader_patch{shader_patch}, _size{size}
   {
      this->resource =
         Index_buffer_view{.buffer = _buffer.get(), .format = DXGI_FORMAT_R32_UINT};
   }

   ~Index_buffer_16_to_32_dynamic() = default;

   D3D11_MAP lock_flags_to_map_type(const DWORD flags) const noexcept
   {
      if (flags & D3DLOCK_READONLY) return D3D11_MAP_READ;
      if (flags & D3DLOCK_DISCARD) return D3D11_MAP_WRITE_DISCARD;
      if (flags & D3DLOCK_NOOVERWRITE) return D3D11_MAP_WRITE_NO_OVERWRITE;

      return D3D11_MAP_WRITE;
   }

   core::Shader_patch& _shader_patch;
   const UINT _size;
   const Com_ptr<ID3D11Buffer> _buffer{
      _shader_patch.create_ia_buffer(_size * 2, false, true, true)};

   int _lock_count = 0;

   DWORD _lock_flags = 0;
   UINT _locked_start = UINT_MAX;
   UINT _locked_end = 0;

   std::vector<std::byte> _cpu_data{_size, std::byte{}};
   ULONG _ref_count = 1;
};

static_assert(
   !std::has_virtual_destructor_v<Index_buffer_16_to_32_managed>,
   "Index_buffer_managed must not have a virtual destructor as it will cause "
   "an ABI break.");

static_assert(
   !std::has_virtual_destructor_v<Index_buffer_16_to_32_dynamic>,
   "Index_buffer_dynamic must not have a virtual destructor as it will cause "
   "an ABI break.");

}
