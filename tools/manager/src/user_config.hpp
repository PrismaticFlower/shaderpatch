#pragma once

#include "framework.hpp"

template<typename T>
struct basic_user_config_value {
   using value_type = T;

   std::wstring_view name = {};
   T value = {};
};

struct bool_user_config_value : basic_user_config_value<bool> {
   winrt::hstring on_display;
   winrt::hstring off_display;
};

struct percentage_user_config_value : basic_user_config_value<std::uint32_t> {
   value_type min_value = {};
};

struct uint2_user_config_value
   : basic_user_config_value<std::array<std::uint32_t, 2>> {
   std::array<winrt::hstring, 2> value_names;
};

struct enum_user_config_value : basic_user_config_value<winrt::hstring> {
   std::vector<winrt::hstring> possible_values;
};

struct string_user_config_value : basic_user_config_value<winrt::hstring> {
};

struct color_user_config_value
   : basic_user_config_value<std::array<std::uint8_t, 3>> {
};

using user_config_value_vector =
   std::vector<std::variant<bool_user_config_value, percentage_user_config_value, uint2_user_config_value,
                            enum_user_config_value, string_user_config_value, color_user_config_value>>;

struct user_config {
   bool_user_config_value enabled{L"Shader Patch Enabled", true, L"Yes", L"No"};

   user_config_value_vector display = {
      percentage_user_config_value{L"Screen Percent", 100, 10},

      percentage_user_config_value{L"Resolution Scale", 100, 50},
      bool_user_config_value{L"Scale UI with Resolution Scale", true, L"Yes", L"No"},

      bool_user_config_value{L"Allow Tearing", false, L"Tearing Allowed",
                             L"Tearing Disallowed"},

      bool_user_config_value{L"Centred", true, L"Centred", L"Uncentred"},
      bool_user_config_value{L"Treat 800x600 As Interface", true, L"Yes", L"No"},
      bool_user_config_value{L"Windowed Interface", false, L"Yes", L"No"},

      bool_user_config_value{L"Display Scaling Aware", true, L"Yes", L"No"},
      bool_user_config_value{L"Display Scaling", true, L"On", L"Off"},

      bool_user_config_value{L"Scalable Fonts", true, L"On", L"Off"},

      bool_user_config_value{L"Enable Game Perceived Resolution Override", false},
      uint2_user_config_value{L"Game Perceived Resolution Override",
                              {1920, 1080},
                              {L"Width", L"Height"}},
   };

   user_config_value_vector user_interface = {
      enum_user_config_value{L"Extra UI Scaling",
                             L"100",
                             {L"100", L"125", L"150", L"175", L"200"}},

      color_user_config_value{L"Friend Color", {1, 86, 213}},

      color_user_config_value{L"Foe Color", {223, 32, 32}},
   };

   user_config_value_vector graphics = {
      enum_user_config_value{L"GPU Selection Method",
                             L"Highest Performance",
                             {L"Highest Performance", L"Lowest Power Usage",
                              L"Highest Feature Level", L"Most Memory", L"Use CPU"}},

      enum_user_config_value{L"Anti-Aliasing Method",
                             L"CMAA2",
                             {L"off", L"CMAA2", L"MSAAx4", L"MSAAx8"}},

      bool_user_config_value{L"Supersample Alpha Test", false, L"Yes", L"No"},

      enum_user_config_value{L"Anisotropic Filtering",
                             L"x16",
                             {L"off", L"x2", L"x4", L"x8", L"x16"}},

      bool_user_config_value{L"Enable 16-Bit Color Channel Rendering", false,
                             L"Enabled", L"Disabled"},

      bool_user_config_value{L"Enable Order-Independent Transparency", false,
                             L"Enabled", L"Disabled"},

      bool_user_config_value{L"Enable Alternative Post Processing", true,
                             L"Enabled", L"Disabled"},

      bool_user_config_value{L"Enable Scene Blur", true, L"Enabled", L"Disabled"},

      enum_user_config_value{L"Refraction Quality",
                             L"Medium",
                             {L"Low", L"Medium", L"High", L"Ultra"}},

      bool_user_config_value{L"Disable Light Brightness Rescaling", false,
                             L"Disabled", L"Enabled"},

      bool_user_config_value{L"Enable User Effects Config", false, L"Enabled", L"Disabled"},

      string_user_config_value{L"User Effects Config", L"user.spfx"},
   };

   user_config_value_vector effects = {
      bool_user_config_value{L"Bloom", true, L"Enabled", L"Disabled"},

      bool_user_config_value{L"Vignette", true, L"Enabled", L"Disabled"},

      bool_user_config_value{L"Film Grain", true, L"Enabled", L"Disabled"},

      bool_user_config_value{L"Allow Colored Film Grain", true, L"Enabled", L"Disabled"},

      bool_user_config_value{L"SSAO", true, L"Enabled", L"Disabled"},

      enum_user_config_value{L"SSAO Quality",
                             L"Medium",
                             {L"Fastest", L"Fast", L"Medium", L"High", L"Highest"}},
   };

   user_config_value_vector developer =
      {string_user_config_value{L"Screen Toggle", L"0xDC"},

       bool_user_config_value{L"Monitor BFront2.log", false, L"Yes", L"No"},

       bool_user_config_value{L"Allow Event Queries", false, L"Yes", L"No"},

       bool_user_config_value{L"Use D3D11 Debug Layer", false, L"Yes", L"No"},

       bool_user_config_value{L"Use DXGI 1.2 Factory", false, L"Yes", L"No"},

       string_user_config_value{L"Shader Cache Path",
                                LR"(.\data\shaderpatch\.shader_dxbc_cache)"},

       string_user_config_value{L"Shader Definitions Path",
                                LR"(.\data\shaderpatch\shaders\definitions)"},

       string_user_config_value{L"Shader Source Path", LR"(.\data\shaderpatch\shaders\src)"},

       string_user_config_value{L"Material Scripts Path",
                                LR"(.\data\shaderpatch\scripts\material)"},

       string_user_config_value{L"Scalable Font Name", L"ariblk.ttf"}};
};
