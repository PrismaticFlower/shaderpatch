#pragma once

#include <d3d9.h>

#include <vector>

namespace sp::direct3d {

class Transform_state_block {
public:
   D3DMATRIX& at(D3DTRANSFORMSTATETYPE transform) noexcept
   {
      switch (transform) {
      case D3DTS_VIEW:
         return _view;
      case D3DTS_PROJECTION:
         return _projection;
      case D3DTS_TEXTURE0:
         return _texture_matrices[0];
      case D3DTS_TEXTURE1:
         return _texture_matrices[1];
      case D3DTS_TEXTURE2:
         return _texture_matrices[2];
      case D3DTS_TEXTURE3:
         return _texture_matrices[3];
      case D3DTS_TEXTURE4:
         return _texture_matrices[4];
      case D3DTS_TEXTURE5:
         return _texture_matrices[5];
      case D3DTS_TEXTURE6:
         return _texture_matrices[6];
      case D3DTS_TEXTURE7:
         return _texture_matrices[7];
      default:
         return _world_matrices[static_cast<int>(transform) - 256];
      }
   }

   const D3DMATRIX& at(D3DTRANSFORMSTATETYPE transform) const noexcept
   {
      switch (transform) {
      case D3DTS_VIEW:
         return _view;
      case D3DTS_PROJECTION:
         return _projection;
      case D3DTS_TEXTURE0:
         return _texture_matrices[0];
      case D3DTS_TEXTURE1:
         return _texture_matrices[1];
      case D3DTS_TEXTURE2:
         return _texture_matrices[2];
      case D3DTS_TEXTURE3:
         return _texture_matrices[3];
      case D3DTS_TEXTURE4:
         return _texture_matrices[4];
      case D3DTS_TEXTURE5:
         return _texture_matrices[5];
      case D3DTS_TEXTURE6:
         return _texture_matrices[6];
      case D3DTS_TEXTURE7:
         return _texture_matrices[7];
      default:
         return _world_matrices[static_cast<int>(transform) - 256];
      }
   }

   void set(D3DTRANSFORMSTATETYPE transform, const D3DMATRIX& matrix) noexcept
   {
      at(transform) = matrix;
   }

   D3DMATRIX get(D3DTRANSFORMSTATETYPE transform) const
   {
      return at(transform);
   }

private:
   D3DMATRIX _view{};
   D3DMATRIX _projection{};
   std::vector<D3DMATRIX> _texture_matrices{8};
   std::vector<D3DMATRIX> _world_matrices{256};
};

inline auto create_filled_transform_state_block(IDirect3DDevice9& device) noexcept
{
   Transform_state_block block;

   device.GetTransform(D3DTS_VIEW, &block.at(D3DTS_VIEW));
   device.GetTransform(D3DTS_PROJECTION, &block.at(D3DTS_PROJECTION));
   device.GetTransform(D3DTS_TEXTURE0, &block.at(D3DTS_TEXTURE0));
   device.GetTransform(D3DTS_TEXTURE1, &block.at(D3DTS_TEXTURE1));
   device.GetTransform(D3DTS_TEXTURE2, &block.at(D3DTS_TEXTURE2));
   device.GetTransform(D3DTS_TEXTURE3, &block.at(D3DTS_TEXTURE3));
   device.GetTransform(D3DTS_TEXTURE4, &block.at(D3DTS_TEXTURE4));
   device.GetTransform(D3DTS_TEXTURE5, &block.at(D3DTS_TEXTURE5));
   device.GetTransform(D3DTS_TEXTURE6, &block.at(D3DTS_TEXTURE6));
   device.GetTransform(D3DTS_TEXTURE7, &block.at(D3DTS_TEXTURE7));

   for (auto i = 0; i < 256; ++i) {
      device.GetTransform(D3DTS_WORLDMATRIX(i), &block.at(D3DTS_WORLDMATRIX(i)));
   }

   return block;
}

inline void apply_transform_state_block(IDirect3DDevice9& device,
                                        const Transform_state_block& block) noexcept
{
   device.SetTransform(D3DTS_VIEW, &block.at(D3DTS_VIEW));
   device.SetTransform(D3DTS_PROJECTION, &block.at(D3DTS_PROJECTION));
   device.SetTransform(D3DTS_TEXTURE0, &block.at(D3DTS_TEXTURE0));
   device.SetTransform(D3DTS_TEXTURE1, &block.at(D3DTS_TEXTURE1));
   device.SetTransform(D3DTS_TEXTURE2, &block.at(D3DTS_TEXTURE2));
   device.SetTransform(D3DTS_TEXTURE3, &block.at(D3DTS_TEXTURE3));
   device.SetTransform(D3DTS_TEXTURE4, &block.at(D3DTS_TEXTURE4));
   device.SetTransform(D3DTS_TEXTURE5, &block.at(D3DTS_TEXTURE5));
   device.SetTransform(D3DTS_TEXTURE6, &block.at(D3DTS_TEXTURE6));
   device.SetTransform(D3DTS_TEXTURE7, &block.at(D3DTS_TEXTURE7));

   for (auto i = 0; i < 256; ++i) {
      device.SetTransform(D3DTS_WORLDMATRIX(i), &block.at(D3DTS_WORLDMATRIX(i)));
   }
}

inline void apply_transform_state_block_partial(IDirect3DDevice9& device,
                                                const Transform_state_block& block) noexcept
{
   device.SetTransform(D3DTS_VIEW, &block.at(D3DTS_VIEW));
   device.SetTransform(D3DTS_PROJECTION, &block.at(D3DTS_PROJECTION));
   device.SetTransform(D3DTS_WORLD, &block.at(D3DTS_WORLD));
}

}
