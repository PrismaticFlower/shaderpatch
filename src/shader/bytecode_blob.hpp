#pragma once

#include "com_ptr.hpp"

#include <cstddef>
#include <memory>

#include <d3dcommon.h>

namespace sp::shader {

class Bytecode_blob {
public:
   Bytecode_blob() = default;

   explicit Bytecode_blob(const std::size_t size)
   {
      _data = std::make_shared<std::byte[]>(size);
      _size = size;
   }

   explicit Bytecode_blob(Com_ptr<ID3DBlob> blob)
   {
      if (!blob) return;

      _data = {std::shared_ptr<ID3DBlob>{blob.unmanaged_copy(),
                                         [](ID3DBlob* blob) { blob->Release(); }},
               static_cast<std::byte*>(blob->GetBufferPointer())};
      _size = blob->GetBufferSize();
   }

   auto data() noexcept -> std::byte*
   {
      return _data.get();
   }

   auto data() const noexcept -> const std::byte*
   {
      return _data.get();
   }

   auto size() const noexcept -> std::size_t
   {
      return _size;
   }

   auto begin() noexcept -> std::byte*
   {
      return _data.get();
   }

   auto end() noexcept -> std::byte*
   {
      return _data.get() + _size;
   }

   auto begin() const noexcept -> const std::byte*
   {
      return _data.get();
   }

   auto end() const noexcept -> const std::byte*
   {
      return _data.get() + _size;
   }

private:
   std::shared_ptr<std::byte[]> _data = nullptr;
   std::size_t _size = 0;
};
}
