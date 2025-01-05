#pragma once

#include "com_ptr.hpp"

#include <span>
#include <vector>

#include <d3d11_2.h>

namespace sp::shadows {

struct Mesh_copy_queue {
   Mesh_copy_queue(ID3D11Device& device);

   void clear() noexcept;

   void queue(std::span<const std::byte> data, ID3D11Buffer& dest_buffer,
              UINT dest_offset) noexcept;

   void process(ID3D11DeviceContext2& dc) noexcept;

private:
   struct Item {
      Com_ptr<ID3D11Buffer> dest_buffer;
      UINT dest_offset = 0;

      Com_ptr<ID3D11Buffer> src_buffer;
   };

   Com_ptr<ID3D11Device> _device;

   std::vector<Item> _queue;
};

}