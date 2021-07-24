#pragma once

#include "com_ptr.hpp"

#include <queue>

#include <d3d11_4.h>

namespace sp::core {

class GPU_pipeline_statistics {
public:
   explicit GPU_pipeline_statistics(Com_ptr<ID3D11Device5> device)
      : _device{device}
   {
   }

   void mark_begin(ID3D11DeviceContext4& dc) noexcept
   {
      const D3D11_QUERY_DESC desc{.Query = D3D11_QUERY_PIPELINE_STATISTICS};

      Com_ptr<ID3D11Query> query;

      _device->CreateQuery(&desc, query.clear_and_assign());

      dc.Begin(query.get());

      _queries.push(query);
   }

   void mark_end(ID3D11DeviceContext4& dc) noexcept
   {
      if (!_queries.empty()) {
         dc.End(_queries.back().get());
      }
   }

   auto data(ID3D11DeviceContext4& dc) noexcept -> D3D11_QUERY_DATA_PIPELINE_STATISTICS
   {
      if (_queries.empty()) return {};

      if (auto result = dc.GetData(_queries.front().get(), &_data,
                                   sizeof(D3D11_QUERY_DATA_PIPELINE_STATISTICS),
                                   D3D11_ASYNC_GETDATA_DONOTFLUSH);
          result == S_OK || FAILED(result)) {
         _queries.pop();
      }

      return _data;
   }

private:
   Com_ptr<ID3D11Device5> _device;
   std::queue<Com_ptr<ID3D11Query>> _queries;
   D3D11_QUERY_DATA_PIPELINE_STATISTICS _data;
};

}
