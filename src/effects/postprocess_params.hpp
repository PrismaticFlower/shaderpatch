#pragma once

#include "glm_yaml_adapters.hpp"

#include <glm/glm.hpp>
#include <string>
#include <string_view>

#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4127)

#include <yaml-cpp/yaml.h>
#pragma warning(pop)

namespace sp::effects {

enum class Hdr_state { hdr, stock };

enum class Tonemapper { filmic, aces_fitted, filmic_heji2015, reinhard, none };

enum class Bloom_mode { blended, threshold };

enum class SSAO_mode { ambient, global };

enum class SSAO_method { assao };

struct Bloom_params {
   bool enabled = true;
   Bloom_mode mode = Bloom_mode::blended;

   float threshold = 1.0f;
   float blend_factor = 0.05f;

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

struct Vignette_params {
   bool enabled = true;

   float end = 1.0f;
   float start = 0.25f;
};

struct Color_grading_params {
   glm::vec3 color_filter = {1.0f, 1.0f, 1.0f};
   float saturation = 1.0f;
   float exposure = 0.0f;
   float brightness = 1.0f;
   float contrast = 1.0f;

   Tonemapper tonemapper = Tonemapper::filmic;

   float filmic_toe_strength = 0.0f;
   float filmic_toe_length = 0.5f;
   float filmic_shoulder_strength = 0.0f;
   float filmic_shoulder_length = 0.5f;
   float filmic_shoulder_angle = 0.0f;

   float filmic_heji_whitepoint = 1.0f;

   glm::vec3 shadow_color = {1.0f, 1.0f, 1.0f};
   glm::vec3 midtone_color = {1.0f, 1.0f, 1.0f};
   glm::vec3 highlight_color = {1.0f, 1.0f, 1.0f};

   float shadow_offset = 0.0f;
   float midtone_offset = 0.0f;
   float highlight_offset = 0.0f;

   float hsv_hue_adjustment = 0.0f;
   float hsv_saturation_adjustment = 1.0f;
   float hsv_value_adjustment = 1.0f;

   glm::vec3 channel_mix_red{1.0f, 0.0f, 0.0f};
   glm::vec3 channel_mix_green{0.0f, 1.0f, 0.0f};
   glm::vec3 channel_mix_blue{0.0f, 0.0f, 1.0f};
};

struct Film_grain_params {
   bool enabled = false;
   bool colored = false;

   float amount = 0.035f;
   float size = 1.65f;
   float color_amount = 0.6f;
   float luma_amount = 1.0f;
};

struct SSAO_params {
   bool enabled = true;

   SSAO_mode mode = SSAO_mode::ambient;
   SSAO_method method = SSAO_method::assao;

   float radius = 1.5f;
   float shadow_multiplier = 0.75f;
   float shadow_power = 0.75f;
   float detail_shadow_strength = 0.5f;
   int blur_pass_count = 2;
   float sharpness = 0.98f;
};

struct FFX_cas_params {
   bool enabled = false;

   float sharpness = 0.0f;
};

struct DOF_params {
   bool enabled = false;

   float film_size_mm = 35.0f;
   float focus_distance = 10.0f;
   float f_stop = 16.0f;
};

inline auto to_string(const Tonemapper tonemapper) noexcept
{
   using namespace std::literals;

   switch (tonemapper) {
   case Tonemapper::filmic:
      return "Filmic"s;
   case Tonemapper::aces_fitted:
      return "ACES sRGB Fitted"s;
   case Tonemapper::filmic_heji2015:
      return "Filmic Heji 2015"s;
   case Tonemapper::reinhard:
      return "Reinhard"s;
   case Tonemapper::none:
      return "None"s;
   }

   std::terminate();
}

inline auto tonemapper_from_string(const std::string_view string) noexcept
{
   if (string == to_string(Tonemapper::filmic))
      return Tonemapper::filmic;
   else if (string == to_string(Tonemapper::aces_fitted))
      return Tonemapper::aces_fitted;
   else if (string == to_string(Tonemapper::filmic_heji2015))
      return Tonemapper::filmic_heji2015;
   else if (string == to_string(Tonemapper::reinhard))
      return Tonemapper::reinhard;
   else if (string == to_string(Tonemapper::none))
      return Tonemapper::none;

   return Tonemapper::filmic;
}

inline auto to_string(const Bloom_mode bloom_mode) noexcept
{
   using namespace std::literals;

   switch (bloom_mode) {
   case Bloom_mode::blended:
      return "Blended"s;
   case Bloom_mode::threshold:
      return "Threshold"s;
   }

   std::terminate();
}

inline auto bloom_mode_from_string(const std::string_view string) noexcept
{
   if (string == to_string(Bloom_mode::blended))
      return Bloom_mode::blended;
   else if (string == to_string(Bloom_mode::threshold))
      return Bloom_mode::threshold;

   return Bloom_mode::blended;
}

inline auto to_string(const SSAO_mode ssao_mode) noexcept
{
   using namespace std::literals;

   switch (ssao_mode) {
   case SSAO_mode::ambient:
      return "Ambient"s;
   case SSAO_mode::global:
      return "Global"s;
   }

   std::terminate();
}

inline auto ssao_mode_from_string(const std::string_view string) noexcept
{
   if (string == to_string(SSAO_mode::ambient))
      return SSAO_mode::ambient;
   else if (string == to_string(SSAO_mode::global))
      return SSAO_mode::global;

   return SSAO_mode::ambient;
}

inline auto to_string(const SSAO_method ssao_type) noexcept
{
   using namespace std::literals;

   switch (ssao_type) {
   case SSAO_method::assao:
      return "ASSAO"s;
   }

   std::terminate();
}

inline auto ssao_type_from_string(const std::string_view string) noexcept
{
   if (string == to_string(SSAO_method::assao)) return SSAO_method::assao;

   return SSAO_method::assao;
}

}

namespace YAML {

template<>
struct convert<sp::effects::Tonemapper> {
   static Node encode(const sp::effects::Tonemapper tonemapper)
   {
      return YAML::Node{to_string(tonemapper)};
   }

   static bool decode(const Node& node, sp::effects::Tonemapper& tonemapper)
   {
      tonemapper = sp::effects::tonemapper_from_string(node.as<std::string>());

      return true;
   }
};

template<>
struct convert<sp::effects::Bloom_mode> {
   static Node encode(const sp::effects::Bloom_mode bloom_mode)
   {
      return YAML::Node{to_string(bloom_mode)};
   }

   static bool decode(const Node& node, sp::effects::Bloom_mode& bloom_mode)
   {
      bloom_mode = sp::effects::bloom_mode_from_string(node.as<std::string>());

      return true;
   }
};

template<>
struct convert<sp::effects::SSAO_mode> {
   static Node encode(const sp::effects::SSAO_mode ssao_mode)
   {
      return YAML::Node{to_string(ssao_mode)};
   }

   static bool decode(const Node& node, sp::effects::SSAO_mode& ssao_mode)
   {
      ssao_mode = sp::effects::ssao_mode_from_string(node.as<std::string>());

      return true;
   }
};

template<>
struct convert<sp::effects::SSAO_method> {
   static Node encode(const sp::effects::SSAO_method ssao_type)
   {
      return YAML::Node{to_string(ssao_type)};
   }

   static bool decode(const Node& node, sp::effects::SSAO_method& ssao_type)
   {
      ssao_type = sp::effects::ssao_type_from_string(node.as<std::string>());

      return true;
   }
};

template<>
struct convert<sp::effects::Bloom_params> {
   static Node encode(const sp::effects::Bloom_params& params)
   {
      using namespace std::literals;

      YAML::Node node;

      node["Enable"s] = params.enabled;
      node["Mode"s] = params.mode;

      node["BlendFactor"s] = params.blend_factor;
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
      params.mode =
         node["Mode"s].as<sp::effects::Bloom_mode>(sp::effects::Bloom_mode::threshold);

      if (params.mode == sp::effects::Bloom_mode::blended)
         params.blend_factor = node["BlendFactor"s].as<float>(params.blend_factor);
      else
         params.threshold = node["Threshold"s].as<float>(params.threshold);

      params.intensity = node["Intensity"s].as<float>(0.75f);

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

      if (params.mode == sp::effects::Bloom_mode::threshold)
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
struct convert<sp::effects::Vignette_params> {
   static Node encode(const sp::effects::Vignette_params& params)
   {
      using namespace std::literals;

      YAML::Node node;

      node["Enable"s] = params.enabled;

      node["End"s] = params.end;
      node["Start"s] = params.start;

      return node;
   }

   static bool decode(const Node& node, sp::effects::Vignette_params& params)
   {
      using namespace std::literals;

      params = sp::effects::Vignette_params{};

      params.enabled = node["Enable"s].as<bool>(params.enabled);

      params.end = node["End"s].as<float>(params.end);
      params.start = node["Start"s].as<float>(params.start);

      return true;
   }
};

template<>
struct convert<sp::effects::Color_grading_params> {
   static Node encode(const sp::effects::Color_grading_params& params)
   {
      using namespace std::literals;

      YAML::Node node;

      node["ColorFilter"s] = params.color_filter;

      node["Saturation"s] = params.saturation;
      node["Exposure"s] = params.exposure;
      node["Brightness"s] = params.brightness;
      node["Contrast"s] = params.contrast;

      node["Tonemapper"s] = params.tonemapper;

      node["FilmicToeStrength"s] = params.filmic_toe_strength;
      node["FilmicToeLength"s] = params.filmic_toe_length;
      node["FilmicShoulderStrength"s] = params.filmic_shoulder_strength;
      node["FilmicShoulderLength"s] = params.filmic_shoulder_length;
      node["FilmicShoulderAngle"s] = params.filmic_shoulder_angle;
      node["FilmicHejiWhitepoint"s] = params.filmic_heji_whitepoint;

      node["ShadowColor"s] = params.shadow_color;
      node["MidtoneColor"s] = params.midtone_color;
      node["HighlightColor"s] = params.highlight_color;

      node["ShadowOffset"s] = params.shadow_offset;
      node["MidtoneOffset"s] = params.midtone_offset;
      node["HighlightOffset"s] = params.highlight_offset;

      node["HSVHueAdjustment"s] = params.hsv_hue_adjustment;
      node["HSVSaturationAdjustment"s] = params.hsv_saturation_adjustment;
      node["HSVValueAdjustment"s] = params.hsv_value_adjustment;

      node["ChannelMixRed"s] = params.channel_mix_red;
      node["ChannelMixGreen"s] = params.channel_mix_green;
      node["ChannelMixBlue"s] = params.channel_mix_blue;

      return node;
   }

   static bool decode(const Node& node, sp::effects::Color_grading_params& params)
   {
      using namespace std::literals;

      params = sp::effects::Color_grading_params{};

      params.color_filter = node["ColorFilter"s].as<glm::vec3>(params.color_filter);

      params.saturation = node["Saturation"s].as<float>(params.saturation);
      params.exposure = node["Exposure"s].as<float>(params.exposure);
      params.brightness = node["Brightness"s].as<float>(params.brightness);
      params.contrast = node["Contrast"s].as<float>(params.contrast);

      params.tonemapper =
         node["Tonemapper"s].as<sp::effects::Tonemapper>(params.tonemapper);

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
      params.filmic_heji_whitepoint =
         node["FilmicHejiWhitepoint"s].as<float>(params.filmic_heji_whitepoint);

      params.shadow_color = node["ShadowColor"s].as<glm::vec3>(params.shadow_color);
      params.midtone_color = node["MidtoneColor"s].as<glm::vec3>(params.midtone_color);
      params.highlight_color =
         node["HighlightColor"s].as<glm::vec3>(params.highlight_color);

      params.shadow_offset = node["ShadowOffset"s].as<float>(params.shadow_offset);
      params.midtone_offset = node["MidtoneOffset"s].as<float>(params.midtone_offset);
      params.highlight_offset =
         node["HighlightOffset"s].as<float>(params.highlight_offset);

      params.hsv_hue_adjustment =
         node["HSVHueAdjustment"s].as<float>(params.hsv_hue_adjustment);
      params.hsv_saturation_adjustment =
         node["HSVSaturationAdjustment"s].as<float>(params.hsv_saturation_adjustment);
      params.hsv_value_adjustment =
         node["HSVValueAdjustment"s].as<float>(params.hsv_value_adjustment);

      params.channel_mix_red =
         node["ChannelMixRed"s].as<glm::vec3>(params.channel_mix_red);
      params.channel_mix_green =
         node["ChannelMixGreen"s].as<glm::vec3>(params.channel_mix_green);
      params.channel_mix_blue =
         node["ChannelMixBlue"s].as<glm::vec3>(params.channel_mix_blue);

      return true;
   }
};

template<>
struct convert<sp::effects::Film_grain_params> {
   static Node encode(const sp::effects::Film_grain_params& params)
   {
      using namespace std::literals;

      YAML::Node node;

      node["Enable"s] = params.enabled;
      node["Colored"s] = params.colored;

      node["Amount"s] = params.amount;
      node["Size"s] = params.size;
      node["ColorAmount"s] = params.color_amount;
      node["LumaAmount"s] = params.luma_amount;

      return node;
   }

   static bool decode(const Node& node, sp::effects::Film_grain_params& params)
   {
      using namespace std::literals;

      params = sp::effects::Film_grain_params{};

      params.enabled = node["Enable"s].as<bool>(params.enabled);
      params.colored = node["Colored"s].as<bool>(params.colored);

      params.amount = node["Amount"s].as<float>(params.amount);
      params.size = node["Size"s].as<float>(params.size);
      params.color_amount = node["ColorAmount"s].as<float>(params.color_amount);
      params.luma_amount = node["LumaAmount"s].as<float>(params.luma_amount);

      return true;
   }
};

template<>
struct convert<sp::effects::SSAO_params> {
   static Node encode(const sp::effects::SSAO_params& params)
   {
      using namespace std::literals;

      YAML::Node node;

      node["Enable"s] = params.enabled;
      node["Mode"s] = params.mode;
      node["Method"s] = params.method;
      node["Radius"s] = params.radius;
      node["Shadow Multiplier"s] = params.shadow_multiplier;
      node["Shadow Power"s] = params.shadow_power;
      node["Detail Shadow Strength"s] = params.detail_shadow_strength;
      node["Blur Amount"s] = params.blur_pass_count;
      node["sharpness"s] = params.sharpness;

      return node;
   }

   static bool decode(const Node& node, sp::effects::SSAO_params& params)
   {
      using namespace std::literals;

      params = sp::effects::SSAO_params{};

      params.enabled = node["Enable"s].as<bool>(false);
      params.mode =
         node["Mode"s].as<sp::effects::SSAO_mode>(sp::effects::SSAO_mode::global);
      params.method =
         node["Method"s].as<sp::effects::SSAO_method>(sp::effects::SSAO_method::assao);
      params.radius = node["Radius"s].as<float>(params.radius);
      params.shadow_multiplier =
         node["Shadow Multiplier"s].as<float>(params.shadow_multiplier);
      params.shadow_power = node["Shadow Power"s].as<float>(params.shadow_power);
      params.detail_shadow_strength =
         node["Detail Shadow Strength"s].as<float>(params.detail_shadow_strength);
      params.blur_pass_count = node["Blur Amount"s].as<int>(params.blur_pass_count);
      params.sharpness = node["sharpness"s].as<float>(params.sharpness);

      return true;
   }
};

template<>
struct convert<sp::effects::FFX_cas_params> {
   static Node encode(const sp::effects::FFX_cas_params& params)
   {
      using namespace std::literals;

      YAML::Node node;

      node["Enable"s] = params.enabled;
      node["Sharpness"s] = params.sharpness;

      return node;
   }

   static bool decode(const Node& node, sp::effects::FFX_cas_params& params)
   {
      using namespace std::literals;

      params = sp::effects::FFX_cas_params{};

      params.enabled = node["Enable"s].as<bool>(params.enabled);
      params.sharpness = node["Sharpness"s].as<float>(params.sharpness);

      return true;
   }
};

template<>
struct convert<sp::effects::DOF_params> {
   static Node encode(const sp::effects::DOF_params& params)
   {
      using namespace std::literals;

      YAML::Node node;

      node["Enable"s] = params.enabled;
      node["FilmSize"s] = params.film_size_mm;
      node["FocusDistance"s] = params.focus_distance;
      node["FStop"s] = params.f_stop;

      return node;
   }

   static bool decode(const Node& node, sp::effects::DOF_params& params)
   {
      using namespace std::literals;

      params = sp::effects::DOF_params{};

      params.enabled = node["Enable"s].as<bool>(params.enabled);
      params.film_size_mm = node["FilmSize"s].as<float>(params.film_size_mm);
      params.focus_distance = node["FocusDistance"s].as<float>(params.focus_distance);
      params.f_stop = node["FStop"s].as<float>(params.f_stop);

      params.film_size_mm = std::max(params.film_size_mm, 1.0f);
      params.focus_distance = std::max(params.focus_distance, 0.0f);
      params.f_stop = std::max(params.f_stop, 1.0f);

      return true;
   }
};

}
