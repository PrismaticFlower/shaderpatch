#pragma once

#include "com_ptr.hpp"
#include "shader_metadata.hpp"

#include <atomic>

#include <d3d9.h>
#include <gsl/gsl>

namespace sp::direct3d {

template<typename Type>
class Shader : public Type {
public:
   Shader(Shader_metadata metadata) noexcept : metadata{metadata} {}

   HRESULT __stdcall QueryInterface(const IID&, void**) noexcept override
   {
      log(Log_level::warning,
          "Unimplemented function \"" __FUNCSIG__ "\" called.");

      return E_NOTIMPL;
   }

   ULONG __stdcall AddRef() noexcept override
   {
      return _ref_count += 1;
   }

   ULONG __stdcall Release() noexcept override
   {
      const auto ref_count = _ref_count -= 1;

      if (ref_count == 0) delete this;

      return ref_count;
   }

   HRESULT __stdcall GetDevice(IDirect3DDevice9**) noexcept override
   {
      log(Log_level::warning,
          "Unimplemented function \"" __FUNCSIG__ "\" called.");

      return E_NOTIMPL;
   }

   HRESULT __stdcall GetFunction(void*, UINT*) noexcept override
   {
      log(Log_level::warning,
          "Unimplemented function \"" __FUNCSIG__ "\" called.");

      return E_NOTIMPL;
   }

   const Shader_metadata metadata;

private:
   ~Shader() = default;

   std::atomic<ULONG> _ref_count{1};
};

template<typename Type>
class Null_shader : public Type {
public:
   HRESULT __stdcall QueryInterface(const IID&, void**) noexcept override
   {
      return E_NOINTERFACE;
   }

   ULONG __stdcall AddRef() noexcept override
   {
      return _ref_count += 1;
   }

   ULONG __stdcall Release() noexcept override
   {
      const auto ref_count = _ref_count -= 1;

      if (ref_count == 0) delete this;

      return ref_count;
   }

   HRESULT __stdcall GetDevice(IDirect3DDevice9**) noexcept override
   {
      return E_NOTIMPL;
   }

   HRESULT __stdcall GetFunction(void*, UINT*) noexcept override
   {
      return E_NOTIMPL;
   }

private:
   ~Null_shader() = default;

   std::atomic<ULONG> _ref_count{1};
};

using Vertex_shader = Shader<IDirect3DVertexShader9>;
}
