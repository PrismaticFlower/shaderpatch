#pragma once

#include "glm_yaml_adapters.hpp"

#include <glm/glm.hpp>

#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4127)

#include <yaml-cpp/yaml.h>
#pragma warning(pop)

namespace sp::effects {

struct Bloom_params {
   bool enabled = true;

   float threshold = 0.5f;

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

      params.end = node["Threshold"s].as<float>(params.end);
      params.start = node["Intensity"s].as<float>(params.start);

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

      node["FilmicToeStrength"s] = params.filmic_toe_strength;
      node["FilmicToeLength"s] = params.filmic_toe_length;
      node["FilmicShoulderStrength"s] = params.filmic_shoulder_strength;
      node["FilmicShoulderLength"s] = params.filmic_shoulder_length;
      node["FilmicShoulderAngle"s] = params.filmic_shoulder_angle;

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

}