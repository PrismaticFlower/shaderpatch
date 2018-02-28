#pragma once

#include "../com_ptr.hpp"
#include "shader_metadata.hpp"

#include <atomic>

#include <d3d9.h>

namespace sp::direct3d {

template<typename Type>
class Shader : public Type {
public:
   Shader(Com_ptr<Type> shader, Shader_metadata metadata) noexcept
      : _interface{std::move(shader)}, metadata{metadata}
   {
   }

   HRESULT __stdcall QueryInterface(const IID& iid, void** object) noexcept override
   {
      return _interface->QueryInterface(iid, object);
   }

   ULONG __stdcall AddRef() noexcept override
   {
      return _ref_count.fetch_add(1);
   }

   ULONG __stdcall Release() noexcept override
   {
      const auto ref_count = _ref_count.fetch_sub(1);

      if (ref_count == 0) delete this;

      return ref_count;
   }

   HRESULT __stdcall GetDevice(IDirect3DDevice9** device) noexcept override
   {
      return _interface->GetDevice(device);
   }

   HRESULT __stdcall GetFunction(void* data, UINT* data_size) noexcept override
   {
      return _interface->GetFunction(data, data_size);
   }

   Type& get() noexcept
   {
      return *_interface;
   }

   Type& get() const noexcept
   {
      return *_interface;
   }

   const Shader_metadata metadata;

private:
   ~Shader() = default;

   const Com_ptr<Type> _interface;

   std::atomic<ULONG> _ref_count{1};
};

using Vertex_shader = Shader<IDirect3DVertexShader9>;
using Pixel_shader = Shader<IDirect3DPixelShader9>;
}
