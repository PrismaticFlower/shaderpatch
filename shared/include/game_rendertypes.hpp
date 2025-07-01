#pragma once

#include "compose_exception.hpp"

#include <stdexcept>
#include <string>
#include <string_view>

namespace sp {

enum class Rendertype {
   decal,
   filtercopy,
   flare,
   hdr,
   _interface,
   normal,
   normalmapadder,
   lightbeam,
   ocean,
   particle,
   perpixeldiffuselighting,
   prereflection,
   rain,
   refraction,
   sample,
   shadowquad,
   shield,
   skyfog,
   specularlighting,
   sprite,
   stencilshadow,
   Terrain2,
   water,
   zprepass,

   // Fixed Func Rendertypes
   fixedfunc_color_fill,
   fixedfunc_damage_overlay,
   fixedfunc_plain_texture,
   fixedfunc_scene_blur,
   fixedfunc_zoom_blur,

   invalid = 0x7fffffff
};

inline Rendertype rendertype_from_string(std::string_view string)
{
   using namespace std::literals;

   // clang-format off

   if (string == "decal"sv) { return Rendertype::decal; }
   if (string == "filtercopy"sv) { return Rendertype::filtercopy; }
   if (string == "flare"sv) { return Rendertype::flare; }
   if (string == "hdr"sv) { return Rendertype::hdr; }
   if (string == "interface"sv) { return Rendertype::_interface; }
   if (string == "normal"sv) { return Rendertype::normal; }
   if (string == "normalmapadder"sv) { return Rendertype::normalmapadder; }
   if (string == "lightbeam"sv) { return Rendertype::lightbeam; }
   if (string == "ocean"sv) { return Rendertype::ocean; }
   if (string == "particle"sv) { return Rendertype::particle; }
   if (string == "perpixeldiffuselighting"sv) { return Rendertype::perpixeldiffuselighting; }
   if (string == "prereflection"sv) { return Rendertype::prereflection; }
   if (string == "rain"sv) { return Rendertype::rain; }
   if (string == "refraction"sv) { return Rendertype::refraction; }
   if (string == "sample"sv) { return Rendertype::sample; }
   if (string == "shadowquad"sv) { return Rendertype::shadowquad; }
   if (string == "shield"sv) { return Rendertype::shield; }
   if (string == "skyfog"sv) { return Rendertype::skyfog; }
   if (string == "specularlighting"sv) { return Rendertype::specularlighting; }
   if (string == "sprite"sv) { return Rendertype::sprite; }
   if (string == "stencilshadow"sv) { return Rendertype::stencilshadow; }
   if (string == "Terrain2"sv) { return Rendertype::Terrain2; }
   if (string == "water"sv) { return Rendertype::water; }
   if (string == "zprepass"sv) { return Rendertype::zprepass; }
   if (string == "fixedfunc_color_fill"sv) { return Rendertype::fixedfunc_color_fill; }
   if (string == "fixedfunc_damage_overlay"sv) { return Rendertype::fixedfunc_damage_overlay; }
   if (string == "fixedfunc_plain_texture"sv) { return Rendertype::fixedfunc_plain_texture; }
   if (string == "fixedfunc_scene_blur"sv) { return Rendertype::fixedfunc_scene_blur; }
   if (string == "fixedfunc_zoom_blur"sv) { return Rendertype::fixedfunc_zoom_blur; }

   // clang-format on

   throw compose_exception<std::invalid_argument>('"', string,
                                                  "\" is not a valid Rendertype"sv);
}

inline auto to_string_view(const Rendertype rendertype) noexcept -> std::string_view
{
   using namespace std::literals;

   // clang-format off
   switch (rendertype) {
      case Rendertype::decal: return "decal"sv;
      case Rendertype::filtercopy: return "filtercopy"sv;
      case Rendertype::flare: return "flare"sv;
      case Rendertype::hdr: return "hdr"sv;
      case Rendertype::_interface: return "interface"sv;
      case Rendertype::normal: return "normal"sv;
      case Rendertype::normalmapadder: return "normalmapadder"sv;
      case Rendertype::lightbeam: return "lightbeam"sv;
      case Rendertype::ocean: return "ocean"sv;
      case Rendertype::particle: return "particle"sv;
      case Rendertype::perpixeldiffuselighting: return "perpixeldiffuselighting"sv;
      case Rendertype::prereflection: return "prereflection"sv;
      case Rendertype::rain: return "rain"sv;
      case Rendertype::refraction: return "refraction"sv;
      case Rendertype::sample: return "sample"sv;
      case Rendertype::shadowquad: return "shadowquad"sv;
      case Rendertype::shield: return "shield"sv;
      case Rendertype::skyfog: return "skyfog"sv;
      case Rendertype::specularlighting: return "specularlighting"sv;
      case Rendertype::sprite: return "sprite"sv;
      case Rendertype::stencilshadow: return "stencilshadow"sv;
      case Rendertype::Terrain2: return "Terrain2"sv;
      case Rendertype::water: return "water"sv;
      case Rendertype::zprepass: return "zprepass"sv;
      case Rendertype::fixedfunc_color_fill: return "fixedfunc_color_fill"sv;
      case Rendertype::fixedfunc_damage_overlay: return "fixedfunc_damage_overlay"sv;
      case Rendertype::fixedfunc_plain_texture: return "fixedfunc_plain_texture"sv;
      case Rendertype::fixedfunc_scene_blur: return "fixedfunc_scene_blur"sv;
      case Rendertype::fixedfunc_zoom_blur: return "fixedfunc_zoom_blur"sv;
   }
   // clang-format on

   std::terminate();
}

inline auto to_wcstring(const Rendertype rendertype) noexcept -> const wchar_t*
{
   // clang-format off
   switch (rendertype) {
      case Rendertype::decal: return L"decal";
      case Rendertype::filtercopy: return L"filtercopy";
      case Rendertype::flare: return L"flare";
      case Rendertype::hdr: return L"hdr";
      case Rendertype::_interface: return L"interface";
      case Rendertype::normal: return L"normal";
      case Rendertype::normalmapadder: return L"normalmapadder";
      case Rendertype::lightbeam: return L"lightbeam";
      case Rendertype::ocean: return L"ocean";
      case Rendertype::particle: return L"particle";
      case Rendertype::perpixeldiffuselighting: return L"perpixeldiffuselighting";
      case Rendertype::prereflection: return L"prereflection";
      case Rendertype::rain: return L"rain";
      case Rendertype::refraction: return L"refraction";
      case Rendertype::sample: return L"sample";
      case Rendertype::shadowquad: return L"shadowquad";
      case Rendertype::shield: return L"shield";
      case Rendertype::skyfog: return L"skyfog";
      case Rendertype::specularlighting: return L"specularlighting";
      case Rendertype::sprite: return L"sprite";
      case Rendertype::stencilshadow: return L"stencilshadow";
      case Rendertype::Terrain2: return L"Terrain2";
      case Rendertype::water: return L"water";
      case Rendertype::zprepass: return L"zprepass";
      case Rendertype::fixedfunc_color_fill: return L"fixedfunc_color_fill";
      case Rendertype::fixedfunc_damage_overlay: return L"fixedfunc_damage_overlay";
      case Rendertype::fixedfunc_plain_texture: return L"fixedfunc_plain_texture";
      case Rendertype::fixedfunc_scene_blur: return L"fixedfunc_scene_blur";
      case Rendertype::fixedfunc_zoom_blur: return L"fixedfunc_zoom_blur";
   }
   // clang-format on

   std::terminate();
}

inline auto to_string(const Rendertype rendertype) noexcept -> std::string
{
   return std::string{to_string_view(rendertype)};
}

}
