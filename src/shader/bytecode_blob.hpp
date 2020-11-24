#pragma once

#include "com_ptr.hpp"

#include <cstddef>

#include <d3dcommon.h>

namespace sp::shader {

class Bytecode_blob {
public:
   Bytecode_blob() = default;

   explicit Bytecode_blob(Com_ptr<ID3DBlob> blob)
   {
      _storage_blob = std::move(blob);

      if (_storage_blob) {
         _data = static_cast<std::byte*>(_storage_blob->GetBufferPointer());
         _size = _storage_blob->GetBufferSize();
      }
   }

   auto data() noexcept -> std::byte*
   {
      return _data;
   }

   auto data() const noexcept -> const std::byte*
   {
      return _data;
   }

   auto size() const noexcept -> std::size_t
   {
      return _size;
   }

   auto begin() noexcept -> std::byte*
   {
      return _data;
   }

   auto end() noexcept -> std::byte*
   {
      return _data + _size;
   }

   auto begin() const noexcept -> const std::byte*
   {
      return _data;
   }

   auto end() const noexcept -> const std::byte*
   {
      return _data + _size;
   }

private:
   std::byte* _data = nullptr;
   std::size_t _size = 0;
   Com_ptr<ID3DBlob> _storage_blob;
};

}
