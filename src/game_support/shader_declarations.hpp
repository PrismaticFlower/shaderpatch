#pragma once

#include "declarations/bloom_stock.hpp"
#include "declarations/decal.hpp"
#include "declarations/filtercopy.hpp"
#include "declarations/flare.hpp"
#include "declarations/interface.hpp"
#include "declarations/lightbeam.hpp"
#include "declarations/normal.hpp"
#include "declarations/normal_terrain.hpp"
#include "declarations/normalmapadder.hpp"
#include "declarations/ocean.hpp"
#include "declarations/particle.hpp"
#include "declarations/perpixeldiffuselighting.hpp"
#include "declarations/prereflection.hpp"
#include "declarations/rain.hpp"
#include "declarations/refraction.hpp"
#include "declarations/sample.hpp"
#include "declarations/shadowquad.hpp"
#include "declarations/shield.hpp"
#include "declarations/skyfog.hpp"
#include "declarations/specularlighting.hpp"
#include "declarations/stencilshadow.hpp"
#include "declarations/water.hpp"
#include "declarations/zprepass.hpp"

namespace sp::game_support {

inline constexpr std::tuple shader_declarations{
   bloom_stock_declaration,    decal_declaration,
   filtercopy_declaration,     flare_declaration,
   interface_declaration,      lightbeam_declaration,
   normal_declaration,         normalmapadder_declaration,
   normal_terrain_declaration, ocean_declaration,
   particle_declaration,       perpixeldiffuselighting_declaration,
   prereflection_declaration,  rain_declaration,
   refraction_declaration,     sample_declaration,
   shadowquad_declaration,     shield_declaration,
   skyfog_declaration,         specularlighting_declaration,
   stencilshadow_declaration,  water_declaration,
   zprepass_declaration};

}
