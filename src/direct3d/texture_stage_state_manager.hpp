#pragma once

#include "../core/shader_patch.hpp"

#include <array>

#include <d3d9.h>

namespace sp::d3d9 {

class Texture_stage_state_manager {
public:
   explicit Texture_stage_state_manager(core::Shader_patch& shader_patch) noexcept;

   void set(const UINT stage, const D3DTEXTURESTAGESTATETYPE state,
            const DWORD value) noexcept;

   DWORD get(const UINT stage, const D3DTEXTURESTAGESTATETYPE state) const noexcept;

   void update(core::Shader_patch& shader_patch, const DWORD texture_factor) const
      noexcept;

private:
   bool is_scene_blur_state() const noexcept;

   bool is_zoom_blur_state(const DWORD texture_factor) const noexcept;

   bool is_damage_overlay_state(const DWORD texture_factor) const noexcept;

   bool is_plain_texture_state(const DWORD texture_factor) const noexcept;

   bool is_color_fill_state(const DWORD texture_factor) const noexcept;

   struct Stage_state {
      DWORD colorop;
      DWORD colorarg1;
      DWORD colorarg2;
      DWORD alphaop;
      DWORD alphaarg1;
      DWORD alphaarg2;
      DWORD bumpenvmat00;
      DWORD bumpenvmat01;
      DWORD bumpenvmat10;
      DWORD bumpenvmat11;
      DWORD texcoord_index;
      DWORD bumpenv_lscale;
      DWORD bumpenv_loffset;
      DWORD texture_transform_flags;
      DWORD colorarg0;
      DWORD alphaarg0;
      DWORD resultarg;
      DWORD constant;
   };

   constexpr static Stage_state default_state = {D3DTOP_MODULATE,
                                                 D3DTA_TEXTURE,
                                                 D3DTA_CURRENT,
                                                 D3DTOP_SELECTARG1,
                                                 D3DTA_TEXTURE,
                                                 D3DTA_CURRENT,
                                                 0,
                                                 0,
                                                 0,
                                                 0,
                                                 0,
                                                 0,
                                                 0,
                                                 D3DTTFF_DISABLE,
                                                 D3DTA_CURRENT,
                                                 D3DTA_CURRENT,
                                                 D3DTA_CURRENT,
                                                 0xffffffff};

   constexpr static auto gen_default_state_disabled(const UINT stage) -> Stage_state
   {
      return Stage_state{D3DTOP_DISABLE,
                         D3DTA_TEXTURE,
                         D3DTA_CURRENT,
                         D3DTOP_DISABLE,
                         D3DTA_TEXTURE,
                         D3DTA_CURRENT,
                         0,
                         0,
                         0,
                         0,
                         stage,
                         0,
                         0,
                         D3DTTFF_DISABLE,
                         D3DTA_CURRENT,
                         D3DTA_CURRENT,
                         D3DTA_CURRENT,
                         0xffffffff};
   };

   std::array<Stage_state, 4> _stages{default_state, gen_default_state_disabled(1),
                                      gen_default_state_disabled(2),
                                      gen_default_state_disabled(3)};

   const std::shared_ptr<core::Game_shader> _color_fill_shader;
   const std::shared_ptr<core::Game_shader> _damage_overlay_shader;
   const std::shared_ptr<core::Game_shader> _plain_texture_shader;
   const std::shared_ptr<core::Game_shader> _scene_blur_shader;
   const std::shared_ptr<core::Game_shader> _zoom_blur_shader;
};

}
