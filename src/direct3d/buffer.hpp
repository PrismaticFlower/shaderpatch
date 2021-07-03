#pragma once

#include "../core/shader_patch.hpp"
#include "../logger.hpp"
#include "debug_trace.hpp"
#include "resource_access.hpp"
#include "upload_scratch_buffer.hpp"

#include <type_traits>
#include <utility>

#include <d3d9.h>

namespace sp::d3d9 {

enum class Buffer_type { vertex_buffer, index_buffer };

template<Buffer_type type>
using D3D9_buffer_t =
   std::conditional_t<type == Buffer_type::vertex_buffer, IDirect3DVertexBuffer9, IDirect3DIndexBuffer9>;

template<Buffer_type buffer_type>
class Basic_buffer final : public D3D9_buffer_t<buffer_type> {
public:
   static Com_ptr<Basic_buffer> create(core::Shader_patch& shader_patch,
                                       const UINT size, const bool dynamic,
                                       const bool dynamic_readable) noexcept
   {
      return Com_ptr{new Basic_buffer{shader_patch, size, dynamic, dynamic_readable}};
   }

   static_assert(buffer_type == Buffer_type::vertex_buffer ||
                 buffer_type == Buffer_type::index_buffer);

   using Description_type =
      std::conditional_t<buffer_type == Buffer_type::vertex_buffer, D3DVERTEXBUFFER_DESC, D3DINDEXBUFFER_DESC>;
   using D3D9_interface = D3D9_buffer_t<buffer_type>;

   Basic_buffer(const Basic_buffer&) = delete;
   Basic_buffer& operator=(const Basic_buffer&) = delete;

   Basic_buffer(Basic_buffer&&) = delete;
   Basic_buffer& operator=(Basic_buffer&&) = delete;

   HRESULT __stdcall QueryInterface(const IID& iid, void** object) noexcept override
   {
      Debug_trace::func(__FUNCSIG__);

      if (!object) return E_INVALIDARG;

      if (iid == IID_IUnknown) {
         *object = static_cast<IUnknown*>(static_cast<D3D9_interface*>(this));
      }
      else if (iid == IID_IDirect3DResource9) {
         *object = static_cast<IDirect3DResource9*>(this);
      }
      else if (iid == __uuidof(D3D9_interface)) {
         *object = static_cast<D3D9_interface*>(this);
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

      return buffer_type == Buffer_type::vertex_buffer ? D3DRTYPE_VERTEXBUFFER
                                                       : D3DRTYPE_INDEXBUFFER;
   }

   virtual HRESULT __stdcall Lock(UINT lock_offset, UINT lock_size, void** data,
                                  DWORD flags) noexcept
   {
      Debug_trace::func(__FUNCSIG__);

      if (_dynamic) [[likely]] {
         return lock_dynamic(lock_offset, lock_size, data, flags);
      }
      else {
         return lock_default(lock_offset, lock_size, data, flags);
      }
   }

   virtual HRESULT __stdcall Unlock() noexcept
   {
      Debug_trace::func(__FUNCSIG__);

      if (_dynamic) [[likely]] {
         return unlock_dynamic();
      }
      else {
         return unlock_default();
      }
   }

   [[deprecated("unimplemented")]] virtual HRESULT __stdcall GetDesc(Description_type*) noexcept
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   auto buffer() const noexcept -> ID3D11Buffer&
   {
      return *_buffer;
   }

private:
   Basic_buffer(core::Shader_patch& shader_patch, const UINT size,
                const bool dynamic, const bool dynamic_readable) noexcept
      : _shader_patch{shader_patch}, _size{size}, _dynamic{dynamic}, _dynamic_readable{dynamic_readable}
   {
   }

   ~Basic_buffer() = default;

   auto lock_default(UINT lock_offset, UINT lock_size, void** data, DWORD flags) noexcept
      -> HRESULT
   {
      assert(!_dynamic);

      if (lock_offset == 0 && lock_size == 0) {
         lock_size = _size;
      }

      if (!data) return D3DERR_INVALIDCALL;
      if (lock_size > _size) return D3DERR_INVALIDCALL;
      if (lock_offset > (_size - lock_size)) return D3DERR_INVALIDCALL;

      if (flags != D3DLOCK_NOSYSLOCK) {
         log_and_terminate("Unexpected buffer lock flags!");
      }

      if (++_lock_count != 1) {
         log_and_terminate("Unexpected buffer lock!");
      }

      _lock_offset = lock_offset;
      _lock_size = lock_size;
      _lock_data = upload_scratch_buffer.lock(_size - lock_offset);

      *data = _lock_data;

      return S_OK;
   }

   auto unlock_default() noexcept -> HRESULT
   {
      assert(!_dynamic);

      if (--_lock_count != 0) {
         log_and_terminate("Unexpected buffer unlock!");
      }

      _shader_patch.update_ia_buffer(*_buffer, std::exchange(_lock_offset, 0),
                                     std::exchange(_lock_size, 0),
                                     std::exchange(_lock_data, nullptr));

      upload_scratch_buffer.unlock();

      return S_OK;
   }

   auto lock_dynamic(UINT lock_offset, UINT lock_size, void** data, DWORD flags) noexcept
      -> HRESULT
   {
      assert(_dynamic);

      if (lock_offset == 0 && lock_size == 0) {
         lock_size = _size;
      }

      if (!data) return D3DERR_INVALIDCALL;
      if (lock_size > _size) return D3DERR_INVALIDCALL;
      if (lock_offset > (_size - lock_size)) return D3DERR_INVALIDCALL;

      if (_lock_count == 0) {
         _lock_data =
            _shader_patch.map_ia_buffer(*_buffer, lock_flags_to_map_type(flags));

         if (!_lock_data) return D3DERR_INVALIDCALL;
      }

      _lock_count += 1;

      *data = _lock_data + lock_offset;

      return S_OK;
   }

   auto unlock_dynamic() noexcept -> HRESULT
   {
      assert(_dynamic);

      if (--_lock_count == 0 && std::exchange(_lock_data, nullptr)) {
         _shader_patch.unmap_ia_buffer(*_buffer);
      }
      else if (_lock_count == -1) {
         log_and_terminate("Unexpected buffer unlock!");
      }

      return S_OK;
   }

   auto lock_flags_to_map_type(const DWORD flags) const noexcept -> D3D11_MAP
   {
      if (flags & D3DLOCK_READONLY) return D3D11_MAP_READ;
      if (flags & D3DLOCK_DISCARD) return D3D11_MAP_WRITE_DISCARD;
      if (flags & D3DLOCK_NOOVERWRITE) return D3D11_MAP_WRITE_NO_OVERWRITE;

      if (_dynamic_readable) {
         return D3D11_MAP_READ_WRITE;
      }
      else {
         return D3D11_MAP_WRITE;
      }
   }

   core::Shader_patch& _shader_patch;
   const UINT _size;
   const bool _dynamic;
   const bool _dynamic_readable = false;
   const Com_ptr<ID3D11Buffer> _buffer{
      _shader_patch.create_ia_buffer(_size, buffer_type == Buffer_type::vertex_buffer,
                                     buffer_type == Buffer_type::index_buffer,
                                     _dynamic)};

   UINT _lock_offset{};
   UINT _lock_size{};
   std::byte* _lock_data = nullptr;
   int _lock_count = 0;

   ULONG _ref_count = 1;
};

using Vertex_buffer = Basic_buffer<Buffer_type::vertex_buffer>;
using Index_buffer = Basic_buffer<Buffer_type::index_buffer>;

}
