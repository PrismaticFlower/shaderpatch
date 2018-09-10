#pragma once

#include "../shader_database.hpp"
#include "../texture_database.hpp"
#include "../user_config.hpp"
#include "color_grading_lut_baker.hpp"
#include "com_ptr.hpp"
#include "com_ref.hpp"
#include "postprocess_params.hpp"
#include "rendertarget_allocator.hpp"

#include <array>
#include <optional>
#include <random>
#include <string>

#include <glm/glm.hpp>

#include <d3d9.h>

namespace sp::effects {

enum class Hdr_state { hdr, stock };

class Postprocess {
public:
   Postprocess(Com_ref<IDirect3DDevice9> device);

   void bloom_params(const Bloom_params& params) noexcept;

   auto bloom_params() const noexcept -> const Bloom_params&
   {
      return _bloom_params;
   }

   void vignette_params(const Vignette_params& params) noexcept;

   auto vignette_params() const noexcept -> const Vignette_params&
   {
      return _vignette_params;
   }

   void color_grading_params(const Color_grading_params& params) noexcept;

   auto color_grading_params() const noexcept -> const Color_grading_params&
   {
      return _color_grading_params;
   }

   void film_grain_params(const Film_grain_params& params) noexcept;

   auto film_grain_params() const noexcept -> const Film_grain_params&
   {
      return _film_grain_params;
   }

   void apply(const Shader_database& shaders, Rendertarget_allocator& allocator,
              const Texture_database& textures, IDirect3DTexture9& input,
              IDirect3DSurface9& output) noexcept;

   void drop_device_resources() noexcept;

   void hdr_state(Hdr_state state) noexcept;

   void user_config(const Effects_user_config& config) noexcept;

private:
   void do_bloom_and_color_grading(const Shader_database& shaders,
                                   Rendertarget_allocator& allocator,
                                   const Texture_database& textures,
                                   IDirect3DTexture9& input,
                                   IDirect3DSurface9& output) noexcept;

   void do_color_grading(const Shader_database& shaders, IDirect3DTexture9& input,
                         IDirect3DSurface9& output) noexcept;

   void do_finalize(const Shader_database& shaders, const Texture_database& textures,
                    IDirect3DTexture9& input, IDirect3DSurface9& output);

   void do_pass(IDirect3DTexture9& input, IDirect3DSurface9& output) noexcept;

   void do_bloom_pass(IDirect3DTexture9& input, IDirect3DSurface9& output) noexcept;

   void linear_resample(IDirect3DSurface9& input, IDirect3DSurface9& output) const
      noexcept;

   void set_shader_constants() noexcept;

   void update_randomness() noexcept;

   void bind_blue_noise_texture(const Texture_database& textures) noexcept;

   void setup_vetrex_stream() noexcept;

   void create_resources() noexcept;

   void update_shaders_names() noexcept;

   Com_ref<IDirect3DDevice9> _device;
   Com_ptr<IDirect3DVertexDeclaration9> _vertex_decl;
   Com_ptr<IDirect3DVertexBuffer9> _vertex_buffer;

   struct {
      struct {
         alignas(16) glm::vec3 global{1.0f, 1.0f, 1.0f};
         float threshold{1.0f};

         alignas(16) glm::vec3 dirt{1.0f, 1.0f, 1.0f};
         float _pad{};
      } bloom;

      struct {
         float exposure;
      } color_grading;

      struct {
         float end;
         float start;
         float _pad{};
      } vignette;

      struct {
         alignas(16) float amount;
         float size;
         float color_amount;
         float luma_amount;
      } film_grain;

      alignas(16) glm::vec4 randomness{1.0f, 1.0f, 1.0f, 1.0f};
   } _constants;

   static_assert(sizeof(decltype(_constants)) == 80);

   glm::vec3 _bloom_inner_scale;
   glm::vec3 _bloom_inner_mid_scale;
   glm::vec3 _bloom_mid_scale;
   glm::vec3 _bloom_outer_mid_scale;
   glm::vec3 _bloom_outer_scale;

   Bloom_params _bloom_params{};
   Vignette_params _vignette_params{};
   Color_grading_params _color_grading_params{};
   Film_grain_params _film_grain_params{};
   Effects_user_config _user_config{};

   Hdr_state _hdr_state = Hdr_state::hdr;

   bool _bloom_enabled = true;
   bool _vignette_enabled = true;
   bool _film_grain_enabled = true;
   bool _colored_film_grain_enabled = true;

   std::string _threshold_shader = "downsample threshold"s;
   std::string _uber_shader = "bloom vignette colorgrade"s;
   std::string _aa_quality = "fastest"s;
   std::string _finalize_shader = "finalize fastest"s;

   std::mt19937 _random_engine{std::random_device{}()};
   std::uniform_real_distribution<float> _random_real_dist{0.0f, 256.0f};
   std::uniform_int<int> _random_int_dist{0, 63};

   Color_grading_lut_baker _color_grading_lut_baker{_device};

   constexpr static auto bloom_sampler_slot = 1;
   constexpr static auto dirt_sampler_slot = 2;
   constexpr static auto lut_sampler_slot = 3;
   constexpr static auto blue_noise_sampler_slot = 4;
};

}
