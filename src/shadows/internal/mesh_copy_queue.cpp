#include "mesh_copy_queue.hpp"

#include "../../logger.hpp"

namespace sp::shadows {

Mesh_copy_queue::Mesh_copy_queue(ID3D11Device& device)
   : _device{copy_raw_com_ptr(device)}
{
}

void Mesh_copy_queue::clear() noexcept
{
   _queue.clear();
}

void Mesh_copy_queue::queue(std::span<const std::byte> data,
                            ID3D11Buffer& dest_buffer, UINT dest_offset) noexcept
{
   Com_ptr<ID3D11Buffer> upload_buffer;

   const D3D11_BUFFER_DESC desc{
      .ByteWidth = data.size(),
      .Usage = D3D11_USAGE_STAGING,
      .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
   };
   const D3D11_SUBRESOURCE_DATA init_data{.pSysMem = data.data(),
                                          .SysMemPitch = data.size(),
                                          .SysMemSlicePitch = data.size()};

   if (FAILED(_device->CreateBuffer(&desc, &init_data,
                                    upload_buffer.clear_and_assign()))) {
      log_and_terminate_fmt("Failed to create scratch buffer for uploading "
                            "shadow world mesh data.");
   }

   _queue.push_back({
      .dest_buffer = copy_raw_com_ptr(dest_buffer),
      .dest_offset = dest_offset,
      .src_buffer = std::move(upload_buffer),
   });
}

void Mesh_copy_queue::process(ID3D11DeviceContext2& dc) noexcept
{
   for (const Item& item : _queue) {
      dc.CopySubresourceRegion1(item.dest_buffer.get(), 0, item.dest_offset, 0,
                                0, item.src_buffer.get(), 0, nullptr,
                                D3D11_COPY_NO_OVERWRITE);
   }

   _queue.clear();
}

}