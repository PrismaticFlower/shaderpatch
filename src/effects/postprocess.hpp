#pragma once

#include "../shader_database.hpp"
#include "../texture_database.hpp"
#include "../user_config.hpp"
#include "com_ptr.hpp"
#include "com_ref.hpp"
#include "rendertarget_allocator.hpp"

#include <array>
#include <optional>
#include <string>

#include <glm/glm.hpp>

#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4127)

#include <yaml-cpp/yaml.h>
#pragma warning(pop)

#include <d3d9.h>

namespace sp::effects {

struct Bloom_params {
   bool enabled = true;

   float threshold = 1.0f;

   float intensity = 1.0f;
   glm::vec3 tint{1.0f, 1.0f, 1.0f};

   float inner_scale = 1.0f;
   glm::vec3 inner_tint{1.0f, 1.0f, 1.0f};

   float inner_mid_scale = 1.0f;
   glm::vec3 inner_mid_tint{1.0f, 1.0f, 1.0f};

   float mid_scale = 1.0f;
   glm::vec3 mid_tint{1.0f, 1.0f, 1.0f};

   float outer_mid_scale = 1.0f;
   glm::vec3 outer_mid_tint{1.0f, 1.0f, 1.0f};

   float outer_scale = 1.0f;
   glm::vec3 outer_tint{1.0f, 1.0f, 1.0f};

   bool use_dirt = false;
   float dirt_scale = 1.0f;
   glm::vec3 dirt_tint{1.0f, 1.0f, 1.0f};
   std::string dirt_texture_name;
};

struct Color_grading_params {
   glm::vec3 color_filter = {1.0f, 1.0f, 1.0f};
   float saturation = 1.0f;
   float exposure = 0.0f;
   float contrast = 1.0f;

   float filmic_toe_strength = 0.0f;
   float filmic_toe_length = 0.5f;
   float filmic_shoulder_strength = 0.0f;
   float filmic_shoulder_length = 0.5f;
   float filmic_shoulder_angle = 0.0f;

   glm::vec3 shadow_color = {1.0f, 1.0f, 1.0f};
   glm::vec3 midtone_color = {1.0f, 1.0f, 1.0f};
   glm::vec3 highlight_color = {1.0f, 1.0f, 1.0f};

   float shadow_offset = 0.0f;
   float midtone_offset = 0.0f;
   float highlight_offset = 0.0f;
};

enum class Hdr_state { hdr, stock };

class Postprocess {
public:
   Postprocess(Com_ref<IDirect3DDevice9> device, Post_aa_quality aa_quality);

   void bloom_params(const Bloom_params& params) noexcept;

   auto bloom_params() const noexcept -> const Bloom_params&
   {
      return _bloom_params;
   }

   void color_grading_params(const Color_grading_params& params) noexcept;

   auto color_grading_params() const noexcept -> const Color_grading_params&
   {
      return _color_grading_params;
   }

   void apply(const Shader_database& shaders, Rendertarget_allocator& allocator,
              const Texture_database& textures, IDirect3DTexture9& input,
              IDirect3DSurface9& output) noexcept;

   void drop_device_resources() noexcept;

   void hdr_state(Hdr_state state) noexcept;

   void aa_quality(Post_aa_quality quality) noexcept;

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

   void do_blur_pass(IDirect3DTexture9& input, IDirect3DSurface9& output,
                     glm::vec2 direction) noexcept;

   void linear_resample(IDirect3DSurface9& input, IDirect3DSurface9& output) const
      noexcept;

   void set_shader_constants() const noexcept;

   void bind_color_grading_luts() noexcept;

   void setup_vetrex_stream() noexcept;

   void create_resources() noexcept;

   Com_ref<IDirect3DDevice9> _device;
   Com_ptr<IDirect3DVertexDeclaration9> _vertex_decl;
   Com_ptr<IDirect3DVertexBuffer9> _vertex_buffer;

   struct {
      struct {
         alignas(16) glm::vec3 global{1.0f, 1.0f, 1.0f};
         float threshold{1.0f};

         alignas(16) glm::vec3 inner{1.0f, 1.0f, 1.0f};
         float _pad1{};

         alignas(16) glm::vec3 inner_mid{1.0f, 1.0f, 1.0f};
         float _pad2{};

         alignas(16) glm::vec3 mid{1.0f, 1.0f, 1.0f};
         float _pad3{};

         alignas(16) glm::vec3 outer_mid{1.0f, 1.0f, 1.0f};
         float _pad4{};

         alignas(16) glm::vec3 outer{1.0f, 1.0f, 1.0f};
         float _pad5{};

         alignas(16) glm::vec3 dirt{1.0f, 1.0f, 1.0f};
         float _pad6{};
      } bloom;

      struct {
         alignas(16) glm::vec3 color_filter_exposure{1.0f, 1.0f, 1.0f};
         float saturation{1.0f};
      } color_grading;
   } _constants;

   static_assert(sizeof(decltype(_constants)) == 128);

   std::optional<std::array<Texture, 3>> _color_luts;

   Bloom_params _bloom_params{};
   Color_grading_params _color_grading_params{};

   Hdr_state _hdr_state = Hdr_state::hdr;
   std::string _hdr_suffix = ""s;
   std::string _aa_suffix = "fastest"s;

   constexpr static auto bloom_sampler_slots_start = 1;
   constexpr static auto dirt_sampler_slot_start = 6;
   constexpr static auto lut_sampler_slots_start = 7;
};

}

namespace YAML {
template<>
struct convert<sp::effects::Bloom_params> {
   static Node encode(const sp::effects::Bloom_params& params)
   {
      using namespace std::literals;

      YAML::Node node;

      node["Enable"s] = params.enabled;

      node["Threshold"s] = params.threshold;
      node["Intensity"s] = params.intensity;

      node["Tint"s].push_back(params.tint.r);
      node["Tint"s].push_back(params.tint.g);
      node["Tint"s].push_back(params.tint.b);

      node["InnerScale"s] = params.inner_scale;
      node["InnerTint"s].push_back(params.inner_tint.r);
      node["InnerTint"s].push_back(params.inner_tint.g);
      node["InnerTint"s].push_back(params.inner_tint.b);

      node["InnerMidScale"s] = params.inner_mid_scale;
      node["InnerMidTint"s].push_back(params.inner_mid_tint.r);
      node["InnerMidTint"s].push_back(params.inner_mid_tint.g);
      node["InnerMidTint"s].push_back(params.inner_mid_tint.b);

      node["MidScale"s] = params.mid_scale;
      node["MidTint"s].push_back(params.mid_tint.r);
      node["MidTint"s].push_back(params.mid_tint.g);
      node["MidTint"s].push_back(params.mid_tint.b);

      node["OuterMidScale"s] = params.outer_mid_scale;
      node["OuterMidTint"s].push_back(params.outer_mid_tint.r);
      node["OuterMidTint"s].push_back(params.outer_mid_tint.g);
      node["OuterMidTint"s].push_back(params.outer_mid_tint.b);

      node["OuterScale"s] = params.outer_scale;
      node["OuterTint"s].push_back(params.outer_tint.r);
      node["OuterTint"s].push_back(params.outer_tint.g);
      node["OuterTint"s].push_back(params.outer_tint.b);

      node["UseDirt"s] = params.use_dirt;
      node["DirtScale"s] = params.dirt_scale;
      node["DirtTint"s].push_back(params.dirt_tint[0]);
      node["DirtTint"s].push_back(params.dirt_tint[1]);
      node["DirtTint"s].push_back(params.dirt_tint[2]);
      node["DirtTextureName"s] = params.dirt_texture_name;

      return node;
   }

   static bool decode(const Node& node, sp::effects::Bloom_params& params)
   {
      using namespace std::literals;

      params = sp::effects::Bloom_params{};

      params.enabled = node["Enable"s].as<bool>(params.enabled);

      params.threshold = node["Threshold"s].as<float>(params.threshold);
      params.intensity = node["Intensity"s].as<float>(params.intensity);

      params.tint[0] = node["Tint"s][0].as<float>(params.tint[0]);
      params.tint[1] = node["Tint"s][1].as<float>(params.tint[1]);
      params.tint[2] = node["Tint"s][2].as<float>(params.tint[2]);

      params.inner_scale = node["InnerScale"s].as<float>(params.inner_scale);
      params.inner_tint[0] = node["InnerTint"s][0].as<float>(params.inner_tint[0]);
      params.inner_tint[1] = node["InnerTint"s][1].as<float>(params.inner_tint[1]);
      params.inner_tint[2] = node["InnerTint"s][2].as<float>(params.inner_tint[2]);

      params.inner_mid_scale =
         node["InnerMidScale"s].as<float>(params.inner_mid_scale);
      params.inner_mid_tint[0] =
         node["InnerMidTint"s][0].as<float>(params.inner_mid_tint[0]);
      params.inner_mid_tint[1] =
         node["InnerMidTint"s][1].as<float>(params.inner_mid_tint[1]);
      params.inner_mid_tint[2] =
         node["InnerMidTint"s][2].as<float>(params.inner_mid_tint[2]);

      params.mid_scale = node["MidScale"s].as<float>(params.mid_scale);
      params.mid_tint[0] = node["MidTint"s][0].as<float>(params.mid_tint[0]);
      params.mid_tint[1] = node["MidTint"s][1].as<float>(params.mid_tint[1]);
      params.mid_tint[2] = node["MidTint"s][2].as<float>(params.mid_tint[2]);

      params.outer_mid_scale =
         node["OuterMidScale"s].as<float>(params.outer_mid_scale);
      params.outer_mid_tint[0] =
         node["OuterMidTint"s][0].as<float>(params.outer_mid_tint[0]);
      params.outer_mid_tint[1] =
         node["OuterMidTint"s][1].as<float>(params.outer_mid_tint[1]);
      params.outer_mid_tint[2] =
         node["OuterMidTint"s][2].as<float>(params.outer_mid_tint[2]);

      params.outer_scale = node["OuterScale"s].as<float>(params.outer_scale);
      params.outer_tint[0] = node["OuterTint"s][0].as<float>(params.outer_tint[0]);
      params.outer_tint[1] = node["OuterTint"s][1].as<float>(params.outer_tint[1]);
      params.outer_tint[2] = node["OuterTint"s][2].as<float>(params.outer_tint[2]);

      params.use_dirt = node["UseDirt"s].as<bool>(params.use_dirt);
      params.dirt_scale = node["DirtScale"s].as<float>(params.dirt_scale);
      params.dirt_tint[0] = node["DirtTint"s][0].as<float>(params.dirt_tint[0]);
      params.dirt_tint[1] = node["DirtTint"s][1].as<float>(params.dirt_tint[1]);
      params.dirt_tint[2] = node["DirtTint"s][2].as<float>(params.dirt_tint[2]);
      params.dirt_texture_name =
         node["DirtTextureName"s].as<std::string>(params.dirt_texture_name);

      return true;
   }
};

template<>
struct convert<sp::effects::Color_grading_params> {
   static Node encode(const sp::effects::Color_grading_params& params)
   {
      using namespace std::literals;

      YAML::Node node;

      node["ColorFilter"s].push_back(params.color_filter.r);
      node["ColorFilter"s].push_back(params.color_filter.g);
      node["ColorFilter"s].push_back(params.color_filter.b);

      node["Saturation"s] = params.saturation;
      node["Exposure"s] = params.exposure;
      node["Contrast"s] = params.contrast;

      node["FilmicToeStrength"s] = params.filmic_toe_strength;
      node["FilmicToeLength"s] = params.filmic_toe_length;
      node["FilmicShoulderStrength"s] = params.filmic_shoulder_strength;
      node["FilmicShoulderLength"s] = params.filmic_shoulder_length;
      node["FilmicShoulderAngle"s] = params.filmic_shoulder_angle;

      node["ShadowColor"s].push_back(params.shadow_color.r);
      node["ShadowColor"s].push_back(params.shadow_color.g);
      node["ShadowColor"s].push_back(params.shadow_color.b);

      node["MidtoneColor"s].push_back(params.midtone_color.r);
      node["MidtoneColor"s].push_back(params.midtone_color.g);
      node["MidtoneColor"s].push_back(params.midtone_color.b);

      node["HighlightColor"s].push_back(params.highlight_color.r);
      node["HighlightColor"s].push_back(params.highlight_color.g);
      node["HighlightColor"s].push_back(params.highlight_color.b);

      node["ShadowOffset"s] = params.shadow_offset;
      node["MidtoneOffset"s] = params.midtone_offset;
      node["HighlightOffset"s] = params.highlight_offset;

      return node;
   }

   static bool decode(const Node& node, sp::effects::Color_grading_params& params)
   {
      using namespace std::literals;

      params = sp::effects::Color_grading_params{};

      params.color_filter.r =
         node["ColorFilter"s][0].as<float>(params.color_filter.r);
      params.color_filter.g =
         node["ColorFilter"s][1].as<float>(params.color_filter.g);
      params.color_filter.b =
         node["ColorFilter"s][2].as<float>(params.color_filter.b);

      params.saturation = node["Saturation"s].as<float>(params.saturation);
      params.exposure = node["Exposure"s].as<float>(params.exposure);
      params.contrast = node["Contrast"s].as<float>(params.contrast);

      params.filmic_toe_strength =
         node["FilmicToeStrength"s].as<float>(params.filmic_toe_strength);
      params.filmic_toe_length =
         node["FilmicToeLength"s].as<float>(params.filmic_toe_length);
      params.filmic_shoulder_strength =
         node["FilmicShoulderStrength"s].as<float>(params.filmic_shoulder_strength);
      params.filmic_shoulder_length =
         node["FilmicShoulderLength"s].as<float>(params.filmic_shoulder_length);
      params.filmic_shoulder_angle =
         node["FilmicShoulderAngle"s].as<float>(params.filmic_shoulder_angle);

      params.shadow_color.r =
         node["ShadowColor"s][0].as<float>(params.shadow_color.r);
      params.shadow_color.g =
         node["ShadowColor"s][1].as<float>(params.shadow_color.g);
      params.shadow_color.b =
         node["ShadowColor"s][2].as<float>(params.shadow_color.b);

      params.midtone_color.r =
         node["MidtoneColor"s][0].as<float>(params.midtone_color.r);
      params.midtone_color.g =
         node["MidtoneColor"s][1].as<float>(params.midtone_color.g);
      params.midtone_color.b =
         node["MidtoneColor"s][2].as<float>(params.midtone_color.b);

      params.highlight_color.r =
         node["HighlightColor"s][0].as<float>(params.highlight_color.r);
      params.highlight_color.g =
         node["HighlightColor"s][1].as<float>(params.highlight_color.g);
      params.highlight_color.b =
         node["HighlightColor"s][2].as<float>(params.highlight_color.b);

      params.shadow_offset = node["ShadowOffset"s].as<float>(params.shadow_offset);
      params.midtone_offset = node["MidtoneOffset"s].as<float>(params.midtone_offset);
      params.highlight_offset =
         node["HighlightOffset"s].as<float>(params.highlight_offset);

      return true;
   }
};
}
