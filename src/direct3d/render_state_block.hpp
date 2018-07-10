#pragma once

#include <d3d9.h>

namespace sp::direct3d {

class Render_state_block {
public:
   template<typename Type = DWORD>
   Type& at(const D3DRENDERSTATETYPE state)
   {
      return reinterpret_cast<Type&>(at_impl(*this, state));
   }

   template<typename Type = DWORD>
   const Type& at(const D3DRENDERSTATETYPE state) const
   {
      return reinterpret_cast<const Type&>(at_impl(*this, state));
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
   template<typename Block>
   static auto& at_impl(Block& block, D3DRENDERSTATETYPE state) noexcept
   {
      switch (state) {
      case D3DRS_ZENABLE:
         return block._zenable;
      case D3DRS_FILLMODE:
         return block._fillmode;
      case D3DRS_SHADEMODE:
         return block._shademode;
      case D3DRS_ZWRITEENABLE:
         return block._zwriteenable;
      case D3DRS_ALPHATESTENABLE:
         return block._alphatestenable;
      case D3DRS_LASTPIXEL:
         return block._lastpixel;
      case D3DRS_SRCBLEND:
         return block._srcblend;
      case D3DRS_DESTBLEND:
         return block._destblend;
      case D3DRS_CULLMODE:
         return block._cullmode;
      case D3DRS_ZFUNC:
         return block._zfunc;
      case D3DRS_ALPHAREF:
         return block._alpharef;
      case D3DRS_ALPHAFUNC:
         return block._alphafunc;
      case D3DRS_DITHERENABLE:
         return block._ditherenable;
      case D3DRS_ALPHABLENDENABLE:
         return block._alphablendenable;
      case D3DRS_FOGENABLE:
         return block._fogenable;
      case D3DRS_SPECULARENABLE:
         return block._specularenable;
      case D3DRS_FOGCOLOR:
         return block._fogcolor;
      case D3DRS_FOGTABLEMODE:
         return block._fogtablemode;
      case D3DRS_FOGSTART:
         return block._fogstart;
      case D3DRS_FOGEND:
         return block._fogend;
      case D3DRS_FOGDENSITY:
         return block._fogdensity;
      case D3DRS_RANGEFOGENABLE:
         return block._rangefogenable;
      case D3DRS_STENCILENABLE:
         return block._stencilenable;
      case D3DRS_STENCILFAIL:
         return block._stencilfail;
      case D3DRS_STENCILZFAIL:
         return block._stencilzfail;
      case D3DRS_STENCILPASS:
         return block._stencilpass;
      case D3DRS_STENCILFUNC:
         return block._stencilfunc;
      case D3DRS_STENCILREF:
         return block._stencilref;
      case D3DRS_STENCILMASK:
         return block._stencilmask;
      case D3DRS_STENCILWRITEMASK:
         return block._stencilwritemask;
      case D3DRS_TEXTUREFACTOR:
         return block._texturefactor;
      case D3DRS_WRAP0:
         return block._wrap0;
      case D3DRS_WRAP1:
         return block._wrap1;
      case D3DRS_WRAP2:
         return block._wrap2;
      case D3DRS_WRAP3:
         return block._wrap3;
      case D3DRS_WRAP4:
         return block._wrap4;
      case D3DRS_WRAP5:
         return block._wrap5;
      case D3DRS_WRAP6:
         return block._wrap6;
      case D3DRS_WRAP7:
         return block._wrap7;
      case D3DRS_CLIPPING:
         return block._clipping;
      case D3DRS_LIGHTING:
         return block._lighting;
      case D3DRS_AMBIENT:
         return block._ambient;
      case D3DRS_FOGVERTEXMODE:
         return block._fogvertexmode;
      case D3DRS_COLORVERTEX:
         return block._colorvertex;
      case D3DRS_LOCALVIEWER:
         return block._localviewer;
      case D3DRS_NORMALIZENORMALS:
         return block._normalizenormals;
      case D3DRS_DIFFUSEMATERIALSOURCE:
         return block._diffusematerialsource;
      case D3DRS_SPECULARMATERIALSOURCE:
         return block._specularmaterialsource;
      case D3DRS_AMBIENTMATERIALSOURCE:
         return block._ambientmaterialsource;
      case D3DRS_EMISSIVEMATERIALSOURCE:
         return block._emissivematerialsource;
      case D3DRS_VERTEXBLEND:
         return block._vertexblend;
      case D3DRS_CLIPPLANEENABLE:
         return block._clipplaneenable;
      case D3DRS_POINTSIZE:
         return block._pointsize;
      case D3DRS_POINTSIZE_MIN:
         return block._pointsize_min;
      case D3DRS_POINTSPRITEENABLE:
         return block._pointspriteenable;
      case D3DRS_POINTSCALEENABLE:
         return block._pointscaleenable;
      case D3DRS_POINTSCALE_A:
         return block._pointscale_a;
      case D3DRS_POINTSCALE_B:
         return block._pointscale_b;
      case D3DRS_POINTSCALE_C:
         return block._pointscale_c;
      case D3DRS_MULTISAMPLEANTIALIAS:
         return block._multisampleantialias;
      case D3DRS_MULTISAMPLEMASK:
         return block._multisamplemask;
      case D3DRS_PATCHEDGESTYLE:
         return block._patchedgestyle;
      case D3DRS_DEBUGMONITORTOKEN:
         return block._debugmonitortoken;
      case D3DRS_POINTSIZE_MAX:
         return block._pointsize_max;
      case D3DRS_INDEXEDVERTEXBLENDENABLE:
         return block._indexedvertexblendenable;
      case D3DRS_COLORWRITEENABLE:
         return block._colorwriteenable;
      case D3DRS_TWEENFACTOR:
         return block._tweenfactor;
      case D3DRS_BLENDOP:
         return block._blendop;
      case D3DRS_POSITIONDEGREE:
         return block._positiondegree;
      case D3DRS_NORMALDEGREE:
         return block._normaldegree;
      case D3DRS_SCISSORTESTENABLE:
         return block._scissortestenable;
      case D3DRS_SLOPESCALEDEPTHBIAS:
         return block._slopescaledepthbias;
      case D3DRS_ANTIALIASEDLINEENABLE:
         return block._antialiasedlineenable;
      case D3DRS_MINTESSELLATIONLEVEL:
         return block._mintessellationlevel;
      case D3DRS_MAXTESSELLATIONLEVEL:
         return block._maxtessellationlevel;
      case D3DRS_ADAPTIVETESS_X:
         return block._adaptivetess_x;
      case D3DRS_ADAPTIVETESS_Y:
         return block._adaptivetess_y;
      case D3DRS_ADAPTIVETESS_Z:
         return block._adaptivetess_z;
      case D3DRS_ADAPTIVETESS_W:
         return block._adaptivetess_w;
      case D3DRS_ENABLEADAPTIVETESSELLATION:
         return block._enableadaptivetessellation;
      case D3DRS_TWOSIDEDSTENCILMODE:
         return block._twosidedstencilmode;
      case D3DRS_CCW_STENCILFAIL:
         return block._ccw_stencilfail;
      case D3DRS_CCW_STENCILZFAIL:
         return block._ccw_stencilzfail;
      case D3DRS_CCW_STENCILPASS:
         return block._ccw_stencilpass;
      case D3DRS_CCW_STENCILFUNC:
         return block._ccw_stencilfunc;
      case D3DRS_COLORWRITEENABLE1:
         return block._colorwriteenable1;
      case D3DRS_COLORWRITEENABLE2:
         return block._colorwriteenable2;
      case D3DRS_COLORWRITEENABLE3:
         return block._colorwriteenable3;
      case D3DRS_BLENDFACTOR:
         return block._blendfactor;
      case D3DRS_SRGBWRITEENABLE:
         return block._srgbwriteenable;
      case D3DRS_DEPTHBIAS:
         return block._depthbias;
      case D3DRS_WRAP8:
         return block._wrap8;
      case D3DRS_WRAP9:
         return block._wrap9;
      case D3DRS_WRAP10:
         return block._wrap10;
      case D3DRS_WRAP11:
         return block._wrap11;
      case D3DRS_WRAP12:
         return block._wrap12;
      case D3DRS_WRAP13:
         return block._wrap13;
      case D3DRS_WRAP14:
         return block._wrap14;
      case D3DRS_WRAP15:
         return block._wrap15;
      case D3DRS_SEPARATEALPHABLENDENABLE:
         return block._separatealphablendenable;
      case D3DRS_SRCBLENDALPHA:
         return block._srcblendalpha;
      case D3DRS_DESTBLENDALPHA:
         return block._destblendalpha;
      case D3DRS_BLENDOPALPHA:
         return block._blendopalpha;
      }

      return block._blendopalpha;
   }

   DWORD _zenable;
   DWORD _fillmode;
   DWORD _shademode;
   DWORD _zwriteenable;
   DWORD _alphatestenable;
   DWORD _lastpixel;
   DWORD _srcblend;
   DWORD _destblend;
   DWORD _cullmode;
   DWORD _zfunc;
   DWORD _alpharef;
   DWORD _alphafunc;
   DWORD _ditherenable;
   DWORD _alphablendenable;
   DWORD _fogenable;
   DWORD _specularenable;
   DWORD _fogcolor;
   DWORD _fogtablemode;
   DWORD _fogstart;
   DWORD _fogend;
   DWORD _fogdensity;
   DWORD _rangefogenable;
   DWORD _stencilenable;
   DWORD _stencilfail;
   DWORD _stencilzfail;
   DWORD _stencilpass;
   DWORD _stencilfunc;
   DWORD _stencilref;
   DWORD _stencilmask;
   DWORD _stencilwritemask;
   DWORD _texturefactor;
   DWORD _wrap0;
   DWORD _wrap1;
   DWORD _wrap2;
   DWORD _wrap3;
   DWORD _wrap4;
   DWORD _wrap5;
   DWORD _wrap6;
   DWORD _wrap7;
   DWORD _clipping;
   DWORD _lighting;
   DWORD _ambient;
   DWORD _fogvertexmode;
   DWORD _colorvertex;
   DWORD _localviewer;
   DWORD _normalizenormals;
   DWORD _diffusematerialsource;
   DWORD _specularmaterialsource;
   DWORD _ambientmaterialsource;
   DWORD _emissivematerialsource;
   DWORD _vertexblend;
   DWORD _clipplaneenable;
   DWORD _pointsize;
   DWORD _pointsize_min;
   DWORD _pointspriteenable;
   DWORD _pointscaleenable;
   DWORD _pointscale_a;
   DWORD _pointscale_b;
   DWORD _pointscale_c;
   DWORD _multisampleantialias;
   DWORD _multisamplemask;
   DWORD _patchedgestyle;
   DWORD _debugmonitortoken;
   DWORD _pointsize_max;
   DWORD _indexedvertexblendenable;
   DWORD _colorwriteenable;
   DWORD _tweenfactor;
   DWORD _blendop;
   DWORD _positiondegree;
   DWORD _normaldegree;
   DWORD _scissortestenable;
   DWORD _slopescaledepthbias;
   DWORD _antialiasedlineenable;
   DWORD _mintessellationlevel;
   DWORD _maxtessellationlevel;
   DWORD _adaptivetess_x;
   DWORD _adaptivetess_y;
   DWORD _adaptivetess_z;
   DWORD _adaptivetess_w;
   DWORD _enableadaptivetessellation;
   DWORD _twosidedstencilmode;
   DWORD _ccw_stencilfail;
   DWORD _ccw_stencilzfail;
   DWORD _ccw_stencilpass;
   DWORD _ccw_stencilfunc;
   DWORD _colorwriteenable1;
   DWORD _colorwriteenable2;
   DWORD _colorwriteenable3;
   DWORD _blendfactor;
   DWORD _srgbwriteenable;
   DWORD _depthbias;
   DWORD _wrap8;
   DWORD _wrap9;
   DWORD _wrap10;
   DWORD _wrap11;
   DWORD _wrap12;
   DWORD _wrap13;
   DWORD _wrap14;
   DWORD _wrap15;
   DWORD _separatealphablendenable;
   DWORD _srcblendalpha;
   DWORD _destblendalpha;
   DWORD _blendopalpha;
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

inline void apply_blend_state(IDirect3DDevice9& device, const Render_state_block& block)
{
   device.SetRenderState(D3DRS_SRCBLEND, block.at(D3DRS_SRCBLEND));
   device.SetRenderState(D3DRS_DESTBLEND, block.at(D3DRS_DESTBLEND));
   device.SetRenderState(D3DRS_BLENDOP, block.at(D3DRS_BLENDOP));
   device.SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,
                         block.at(D3DRS_SEPARATEALPHABLENDENABLE));
   device.SetRenderState(D3DRS_ALPHABLENDENABLE, block.at(D3DRS_ALPHABLENDENABLE));
   device.SetRenderState(D3DRS_SRCBLENDALPHA, block.at(D3DRS_SRCBLENDALPHA));
   device.SetRenderState(D3DRS_DESTBLENDALPHA, block.at(D3DRS_DESTBLENDALPHA));
   device.SetRenderState(D3DRS_BLENDOPALPHA, block.at(D3DRS_BLENDOPALPHA));
   device.SetRenderState(D3DRS_COLORWRITEENABLE, block.at(D3DRS_COLORWRITEENABLE));
   device.SetRenderState(D3DRS_SRGBWRITEENABLE, block.at(D3DRS_SRGBWRITEENABLE));
}
}
