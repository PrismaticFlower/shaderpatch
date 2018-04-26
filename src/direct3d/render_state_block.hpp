#pragma once

#include <array>

#include <d3d9.h>

namespace sp::direct3d {

class Render_state_block {
public:
   template<typename Type = DWORD>
   Type& at(const D3DRENDERSTATETYPE state)
   {
      auto index = static_cast<DWORD>(state);

      if (index & 128u) index >>= 1u;

      return reinterpret_cast<Type&>(_state_block[index]);
   }

   template<typename Type = DWORD>
   const Type& at(const D3DRENDERSTATETYPE state) const
   {
      auto index = static_cast<DWORD>(state);

      if (index & 128u) index >>= 1u;

      return reinterpret_cast<const Type&>(_state_block[index]);
   }

   template<typename Type = DWORD>
   Type get(const D3DRENDERSTATETYPE state) const
   {
      return at<Type>(state);
   }

   template<typename Type = DWORD>
   Type set(const D3DRENDERSTATETYPE state, const Type value)
   {
      return at<Type>(state) = value;
   }

private:
   std::array<DWORD, 105> _state_block;
};

inline Render_state_block create_filled_render_state_block(IDirect3DDevice9& device) noexcept
{
   Render_state_block block;

   device.GetRenderState(D3DRS_ZENABLE, &block.at(D3DRS_ZENABLE));
   device.GetRenderState(D3DRS_FILLMODE, &block.at(D3DRS_FILLMODE));
   device.GetRenderState(D3DRS_SHADEMODE, &block.at(D3DRS_SHADEMODE));
   device.GetRenderState(D3DRS_ZWRITEENABLE, &block.at(D3DRS_ZWRITEENABLE));
   device.GetRenderState(D3DRS_ALPHATESTENABLE, &block.at(D3DRS_ALPHATESTENABLE));
   device.GetRenderState(D3DRS_LASTPIXEL, &block.at(D3DRS_LASTPIXEL));
   device.GetRenderState(D3DRS_SRCBLEND, &block.at(D3DRS_SRCBLEND));
   device.GetRenderState(D3DRS_DESTBLEND, &block.at(D3DRS_DESTBLEND));
   device.GetRenderState(D3DRS_CULLMODE, &block.at(D3DRS_CULLMODE));
   device.GetRenderState(D3DRS_ZFUNC, &block.at(D3DRS_ZFUNC));
   device.GetRenderState(D3DRS_ALPHAREF, &block.at(D3DRS_ALPHAREF));
   device.GetRenderState(D3DRS_ALPHAFUNC, &block.at(D3DRS_ALPHAFUNC));
   device.GetRenderState(D3DRS_DITHERENABLE, &block.at(D3DRS_DITHERENABLE));
   device.GetRenderState(D3DRS_ALPHABLENDENABLE, &block.at(D3DRS_ALPHABLENDENABLE));
   device.GetRenderState(D3DRS_FOGENABLE, &block.at(D3DRS_FOGENABLE));
   device.GetRenderState(D3DRS_SPECULARENABLE, &block.at(D3DRS_SPECULARENABLE));
   device.GetRenderState(D3DRS_FOGCOLOR, &block.at(D3DRS_FOGCOLOR));
   device.GetRenderState(D3DRS_FOGTABLEMODE, &block.at(D3DRS_FOGTABLEMODE));
   device.GetRenderState(D3DRS_FOGSTART, &block.at(D3DRS_FOGSTART));
   device.GetRenderState(D3DRS_FOGEND, &block.at(D3DRS_FOGEND));
   device.GetRenderState(D3DRS_FOGDENSITY, &block.at(D3DRS_FOGDENSITY));
   device.GetRenderState(D3DRS_RANGEFOGENABLE, &block.at(D3DRS_RANGEFOGENABLE));
   device.GetRenderState(D3DRS_STENCILENABLE, &block.at(D3DRS_STENCILENABLE));
   device.GetRenderState(D3DRS_STENCILFAIL, &block.at(D3DRS_STENCILFAIL));
   device.GetRenderState(D3DRS_STENCILZFAIL, &block.at(D3DRS_STENCILZFAIL));
   device.GetRenderState(D3DRS_STENCILPASS, &block.at(D3DRS_STENCILPASS));
   device.GetRenderState(D3DRS_STENCILFUNC, &block.at(D3DRS_STENCILFUNC));
   device.GetRenderState(D3DRS_STENCILREF, &block.at(D3DRS_STENCILREF));
   device.GetRenderState(D3DRS_STENCILMASK, &block.at(D3DRS_STENCILMASK));
   device.GetRenderState(D3DRS_STENCILWRITEMASK, &block.at(D3DRS_STENCILWRITEMASK));
   device.GetRenderState(D3DRS_TEXTUREFACTOR, &block.at(D3DRS_TEXTUREFACTOR));
   device.GetRenderState(D3DRS_WRAP0, &block.at(D3DRS_WRAP0));
   device.GetRenderState(D3DRS_WRAP1, &block.at(D3DRS_WRAP1));
   device.GetRenderState(D3DRS_WRAP2, &block.at(D3DRS_WRAP2));
   device.GetRenderState(D3DRS_WRAP3, &block.at(D3DRS_WRAP3));
   device.GetRenderState(D3DRS_WRAP4, &block.at(D3DRS_WRAP4));
   device.GetRenderState(D3DRS_WRAP5, &block.at(D3DRS_WRAP5));
   device.GetRenderState(D3DRS_WRAP6, &block.at(D3DRS_WRAP6));
   device.GetRenderState(D3DRS_WRAP7, &block.at(D3DRS_WRAP7));
   device.GetRenderState(D3DRS_CLIPPING, &block.at(D3DRS_CLIPPING));
   device.GetRenderState(D3DRS_LIGHTING, &block.at(D3DRS_LIGHTING));
   device.GetRenderState(D3DRS_AMBIENT, &block.at(D3DRS_AMBIENT));
   device.GetRenderState(D3DRS_FOGVERTEXMODE, &block.at(D3DRS_FOGVERTEXMODE));
   device.GetRenderState(D3DRS_COLORVERTEX, &block.at(D3DRS_COLORVERTEX));
   device.GetRenderState(D3DRS_LOCALVIEWER, &block.at(D3DRS_LOCALVIEWER));
   device.GetRenderState(D3DRS_NORMALIZENORMALS, &block.at(D3DRS_NORMALIZENORMALS));
   device.GetRenderState(D3DRS_DIFFUSEMATERIALSOURCE,
                         &block.at(D3DRS_DIFFUSEMATERIALSOURCE));
   device.GetRenderState(D3DRS_SPECULARMATERIALSOURCE,
                         &block.at(D3DRS_SPECULARMATERIALSOURCE));
   device.GetRenderState(D3DRS_AMBIENTMATERIALSOURCE,
                         &block.at(D3DRS_AMBIENTMATERIALSOURCE));
   device.GetRenderState(D3DRS_EMISSIVEMATERIALSOURCE,
                         &block.at(D3DRS_EMISSIVEMATERIALSOURCE));
   device.GetRenderState(D3DRS_VERTEXBLEND, &block.at(D3DRS_VERTEXBLEND));
   device.GetRenderState(D3DRS_CLIPPLANEENABLE, &block.at(D3DRS_CLIPPLANEENABLE));
   device.GetRenderState(D3DRS_POINTSIZE, &block.at(D3DRS_POINTSIZE));
   device.GetRenderState(D3DRS_POINTSIZE_MIN, &block.at(D3DRS_POINTSIZE_MIN));
   device.GetRenderState(D3DRS_POINTSPRITEENABLE, &block.at(D3DRS_POINTSPRITEENABLE));
   device.GetRenderState(D3DRS_POINTSCALEENABLE, &block.at(D3DRS_POINTSCALEENABLE));
   device.GetRenderState(D3DRS_POINTSCALE_A, &block.at(D3DRS_POINTSCALE_A));
   device.GetRenderState(D3DRS_POINTSCALE_B, &block.at(D3DRS_POINTSCALE_B));
   device.GetRenderState(D3DRS_POINTSCALE_C, &block.at(D3DRS_POINTSCALE_C));
   device.GetRenderState(D3DRS_MULTISAMPLEANTIALIAS,
                         &block.at(D3DRS_MULTISAMPLEANTIALIAS));
   device.GetRenderState(D3DRS_MULTISAMPLEMASK, &block.at(D3DRS_MULTISAMPLEMASK));
   device.GetRenderState(D3DRS_PATCHEDGESTYLE, &block.at(D3DRS_PATCHEDGESTYLE));
   device.GetRenderState(D3DRS_DEBUGMONITORTOKEN, &block.at(D3DRS_DEBUGMONITORTOKEN));
   device.GetRenderState(D3DRS_POINTSIZE_MAX, &block.at(D3DRS_POINTSIZE_MAX));
   device.GetRenderState(D3DRS_INDEXEDVERTEXBLENDENABLE,
                         &block.at(D3DRS_INDEXEDVERTEXBLENDENABLE));
   device.GetRenderState(D3DRS_COLORWRITEENABLE, &block.at(D3DRS_COLORWRITEENABLE));
   device.GetRenderState(D3DRS_TWEENFACTOR, &block.at(D3DRS_TWEENFACTOR));
   device.GetRenderState(D3DRS_BLENDOP, &block.at(D3DRS_BLENDOP));
   device.GetRenderState(D3DRS_POSITIONDEGREE, &block.at(D3DRS_POSITIONDEGREE));
   device.GetRenderState(D3DRS_NORMALDEGREE, &block.at(D3DRS_NORMALDEGREE));
   device.GetRenderState(D3DRS_SCISSORTESTENABLE, &block.at(D3DRS_SCISSORTESTENABLE));
   device.GetRenderState(D3DRS_SLOPESCALEDEPTHBIAS,
                         &block.at(D3DRS_SLOPESCALEDEPTHBIAS));
   device.GetRenderState(D3DRS_ANTIALIASEDLINEENABLE,
                         &block.at(D3DRS_ANTIALIASEDLINEENABLE));
   device.GetRenderState(D3DRS_MINTESSELLATIONLEVEL,
                         &block.at(D3DRS_MINTESSELLATIONLEVEL));
   device.GetRenderState(D3DRS_MAXTESSELLATIONLEVEL,
                         &block.at(D3DRS_MAXTESSELLATIONLEVEL));
   device.GetRenderState(D3DRS_ADAPTIVETESS_X, &block.at(D3DRS_ADAPTIVETESS_X));
   device.GetRenderState(D3DRS_ADAPTIVETESS_Y, &block.at(D3DRS_ADAPTIVETESS_Y));
   device.GetRenderState(D3DRS_ADAPTIVETESS_Z, &block.at(D3DRS_ADAPTIVETESS_Z));
   device.GetRenderState(D3DRS_ADAPTIVETESS_W, &block.at(D3DRS_ADAPTIVETESS_W));
   device.GetRenderState(D3DRS_ENABLEADAPTIVETESSELLATION,
                         &block.at(D3DRS_ENABLEADAPTIVETESSELLATION));
   device.GetRenderState(D3DRS_TWOSIDEDSTENCILMODE,
                         &block.at(D3DRS_TWOSIDEDSTENCILMODE));
   device.GetRenderState(D3DRS_CCW_STENCILFAIL, &block.at(D3DRS_CCW_STENCILFAIL));
   device.GetRenderState(D3DRS_CCW_STENCILZFAIL, &block.at(D3DRS_CCW_STENCILZFAIL));
   device.GetRenderState(D3DRS_CCW_STENCILPASS, &block.at(D3DRS_CCW_STENCILPASS));
   device.GetRenderState(D3DRS_CCW_STENCILFUNC, &block.at(D3DRS_CCW_STENCILFUNC));
   device.GetRenderState(D3DRS_COLORWRITEENABLE1, &block.at(D3DRS_COLORWRITEENABLE1));
   device.GetRenderState(D3DRS_COLORWRITEENABLE2, &block.at(D3DRS_COLORWRITEENABLE2));
   device.GetRenderState(D3DRS_COLORWRITEENABLE3, &block.at(D3DRS_COLORWRITEENABLE3));
   device.GetRenderState(D3DRS_BLENDFACTOR, &block.at(D3DRS_BLENDFACTOR));
   device.GetRenderState(D3DRS_SRGBWRITEENABLE, &block.at(D3DRS_SRGBWRITEENABLE));
   device.GetRenderState(D3DRS_DEPTHBIAS, &block.at(D3DRS_DEPTHBIAS));
   device.GetRenderState(D3DRS_WRAP8, &block.at(D3DRS_WRAP8));
   device.GetRenderState(D3DRS_WRAP9, &block.at(D3DRS_WRAP9));
   device.GetRenderState(D3DRS_WRAP10, &block.at(D3DRS_WRAP10));
   device.GetRenderState(D3DRS_WRAP11, &block.at(D3DRS_WRAP11));
   device.GetRenderState(D3DRS_WRAP12, &block.at(D3DRS_WRAP12));
   device.GetRenderState(D3DRS_WRAP13, &block.at(D3DRS_WRAP13));
   device.GetRenderState(D3DRS_WRAP14, &block.at(D3DRS_WRAP14));
   device.GetRenderState(D3DRS_WRAP15, &block.at(D3DRS_WRAP15));
   device.GetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,
                         &block.at(D3DRS_SEPARATEALPHABLENDENABLE));
   device.GetRenderState(D3DRS_SRCBLENDALPHA, &block.at(D3DRS_SRCBLENDALPHA));
   device.GetRenderState(D3DRS_DESTBLENDALPHA, &block.at(D3DRS_DESTBLENDALPHA));
   device.GetRenderState(D3DRS_BLENDOPALPHA, &block.at(D3DRS_BLENDOPALPHA));

   return block;
}
}
