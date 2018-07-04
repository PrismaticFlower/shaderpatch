
#include <array>

#include <d3d9.h>

namespace sp::direct3d {

class Texture_state_block {
public:
   template<typename Type = DWORD>
   Type& at(const D3DTEXTURESTAGESTATETYPE state)
   {
      const auto index = static_cast<DWORD>(state) - 1;

      return reinterpret_cast<Type&>(_state_block[index]);
   }

   template<typename Type = DWORD>
   const Type& at(const D3DTEXTURESTAGESTATETYPE state) const
   {
      const auto index = static_cast<DWORD>(state) - 1;

      return reinterpret_cast<const Type&>(_state_block[index]);
   }

   template<typename Type = DWORD>
   Type get(const D3DTEXTURESTAGESTATETYPE state) const
   {
      return at<Type>(state);
   }

   template<typename Type = DWORD>
   Type set(const D3DTEXTURESTAGESTATETYPE state, const Type value)
   {
      return at<Type>(state) = value;
   }

private:
   std::array<DWORD, 32> _state_block;
};

inline auto create_filled_texture_state_block(IDirect3DDevice9& device, int stage) noexcept
{
   Texture_state_block block;

   device.GetTextureStageState(stage, D3DTSS_COLOROP, &block.at(D3DTSS_COLOROP));
   device.GetTextureStageState(stage, D3DTSS_COLORARG1, &block.at(D3DTSS_COLORARG1));
   device.GetTextureStageState(stage, D3DTSS_COLORARG2, &block.at(D3DTSS_COLORARG2));
   device.GetTextureStageState(stage, D3DTSS_ALPHAOP, &block.at(D3DTSS_ALPHAOP));
   device.GetTextureStageState(stage, D3DTSS_ALPHAARG1, &block.at(D3DTSS_ALPHAARG1));
   device.GetTextureStageState(stage, D3DTSS_ALPHAARG2, &block.at(D3DTSS_ALPHAARG2));
   device.GetTextureStageState(stage, D3DTSS_BUMPENVMAT00,
                               &block.at(D3DTSS_BUMPENVMAT00));
   device.GetTextureStageState(stage, D3DTSS_BUMPENVMAT01,
                               &block.at(D3DTSS_BUMPENVMAT01));
   device.GetTextureStageState(stage, D3DTSS_BUMPENVMAT10,
                               &block.at(D3DTSS_BUMPENVMAT10));
   device.GetTextureStageState(stage, D3DTSS_BUMPENVMAT11,
                               &block.at(D3DTSS_BUMPENVMAT11));
   device.GetTextureStageState(stage, D3DTSS_TEXCOORDINDEX,
                               &block.at(D3DTSS_TEXCOORDINDEX));
   device.GetTextureStageState(stage, D3DTSS_BUMPENVLSCALE,
                               &block.at(D3DTSS_BUMPENVLSCALE));
   device.GetTextureStageState(stage, D3DTSS_BUMPENVLOFFSET,
                               &block.at(D3DTSS_BUMPENVLOFFSET));
   device.GetTextureStageState(stage, D3DTSS_TEXTURETRANSFORMFLAGS,
                               &block.at(D3DTSS_TEXTURETRANSFORMFLAGS));
   device.GetTextureStageState(stage, D3DTSS_COLORARG0, &block.at(D3DTSS_COLORARG0));
   device.GetTextureStageState(stage, D3DTSS_ALPHAARG0, &block.at(D3DTSS_ALPHAARG0));
   device.GetTextureStageState(stage, D3DTSS_RESULTARG, &block.at(D3DTSS_RESULTARG));
   device.GetTextureStageState(stage, D3DTSS_CONSTANT, &block.at(D3DTSS_CONSTANT));

   return block;
}

inline auto create_filled_texture_state_blocks(IDirect3DDevice9& device) noexcept
   -> std::array<Texture_state_block, 8>
{
   std::array<Texture_state_block, 8> blocks;

   for (auto i = 0; i < 8; ++i) {
      blocks[i] = create_filled_texture_state_block(device, i);
   }

   return blocks;
}

}
