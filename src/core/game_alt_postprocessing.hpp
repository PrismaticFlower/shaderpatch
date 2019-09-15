#pragma once

#include "../effects/rendertarget_allocator.hpp"
#include "com_ptr.hpp"
#include "game_rendertarget.hpp"
#include "postprocessing/bloom.hpp"
#include "postprocessing/scene_blur.hpp"
#include "postprocessing/scope_blur.hpp"

#include <d3d11_4.h>

namespace sp::core {

class Game_alt_postprocessing {
public:
   Game_alt_postprocessing(ID3D11Device5& device,
                           const Shader_database& shader_database) noexcept;

   ~Game_alt_postprocessing() = default;
   Game_alt_postprocessing(const Game_alt_postprocessing&) = default;
   Game_alt_postprocessing& operator=(const Game_alt_postprocessing&) = default;
   Game_alt_postprocessing(Game_alt_postprocessing&&) = default;
   Game_alt_postprocessing& operator=(Game_alt_postprocessing&&) = default;

   void blur_factor(const float factor) noexcept;

   void bloom_threshold(const float threshold) noexcept;

   void bloom_intensity(const float intensity) noexcept;

   void apply_scene_blur(ID3D11DeviceContext4& dc, const Game_rendertarget& src_dest,
                         effects::Rendertarget_allocator& rt_allocator) noexcept;

   void apply_scope_blur(ID3D11DeviceContext4& dc, const Game_rendertarget& src_dest,
                         effects::Rendertarget_allocator& rt_allocator) noexcept;

   void apply_bloom(ID3D11DeviceContext4& dc, const Game_rendertarget& src_dest,
                    effects::Rendertarget_allocator& rt_allocator) noexcept;

   void end_frame() noexcept;

private:
   void update_texture(ID3D11DeviceContext4& dc,
                       const Game_rendertarget& game_texture) noexcept;

   bool _generated_mips = false;

   Com_ptr<ID3D11Texture2D> _game_texture;
   Com_ptr<ID3D11Texture2D> _mips_texture;
   Com_ptr<ID3D11ShaderResourceView> _mips_srv;
   Com_ptr<ID3D11ShaderResourceView> _lowest_mip_srv;

   std::uint16_t _lowest_mip_width = 0;
   std::uint16_t _lowest_mip_height = 0;

   postprocessing::Scene_blur _scene_blur;
   postprocessing::Scope_blur _scope_blur;
   postprocessing::Bloom _bloom;

   Com_ptr<ID3D11Device5> _device;
};

}
#pragma once
