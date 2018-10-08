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
   zprepass
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

   // clang-format on

   throw compose_exception<std::invalid_argument>('"', string,
                                                  "\" is not a valid Rendertype"sv);
}

inline std::string to_string(const Rendertype rendertype)
{
   using namespace std::literals;

   // clang-format off
   switch (rendertype) {
      case Rendertype::decal: return "decal"s;
      case Rendertype::filtercopy: return "filtercopy"s;
      case Rendertype::flare: return "flare"s;
      case Rendertype::hdr: return "hdr"s;
      case Rendertype::_interface: return "interface"s;
      case Rendertype::normal: return "normal"s;
      case Rendertype::normalmapadder: return "normalmapadder"s;
      case Rendertype::lightbeam: return "lightbeam"s;
      case Rendertype::ocean: return "ocean"s;
      case Rendertype::particle: return "particle"s;
      case Rendertype::perpixeldiffuselighting: return "perpixeldiffuselighting"s;
      case Rendertype::prereflection: return "prereflection"s;
      case Rendertype::rain: return "rain"s;
      case Rendertype::refraction: return "refraction"s;
      case Rendertype::sample: return "sample"s;
      case Rendertype::shadowquad: return "shadowquad"s;
      case Rendertype::shield: return "shield"s;
      case Rendertype::skyfog: return "skyfog"s;
      case Rendertype::specularlighting: return "specularlighting"s;
      case Rendertype::sprite: return "sprite"s;
      case Rendertype::stencilshadow: return "stencilshadow"s;
      case Rendertype::Terrain2: return "Terrain2"s;
      case Rendertype::water: return "water"s;
      case Rendertype::zprepass: return "zprepass"s;
   }
   // clang-format on
}

}
