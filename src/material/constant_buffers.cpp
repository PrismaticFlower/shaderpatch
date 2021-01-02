
#include "constant_buffers.hpp"
#include "../core/d3d11_helpers.hpp"
#include "compose_exception.hpp"
#include "constant_buffer_builder.hpp"

#include <iomanip>

#include <fmt/format.h>

using namespace std::literals;

namespace sp::material {

namespace {

constexpr std::size_t terrain_texture_count = 16;

template<typename Type>
auto apply_op(const Type value, [[maybe_unused]] const Material_property_var_op op) noexcept
{
   if constexpr (std::is_same_v<Type, float> || std::is_same_v<Type, glm::vec2> ||
                 std::is_same_v<Type, glm::vec3> || std::is_same_v<Type, glm::vec4>) {
      switch (op) {
      case Material_property_var_op::sqr:
         return value * value;
      case Material_property_var_op::sqrt:
         return glm::sqrt(value);
      case Material_property_var_op::exp:
         return glm::exp(value);
      case Material_property_var_op::exp2:
         return glm::exp2(value);
      case Material_property_var_op::log:
         return glm::log(value);
      case Material_property_var_op::log2:
         return glm::log2(value);
      case Material_property_var_op::sign:
         return glm::sign(value);
      case Material_property_var_op::rcp:
         return 1.0f / value;
      }
   }

   return value;
}

class Material_properties_view {
public:
   explicit Material_properties_view(const std::vector<Material_property>& properties) noexcept
      : _properties{properties}
   {
   }

   Material_properties_view(const Material_properties_view&) = delete;
   Material_properties_view& operator=(const Material_properties_view&) = delete;

   Material_properties_view(Material_properties_view&&) = delete;
   Material_properties_view& operator=(Material_properties_view&&) = delete;

   ~Material_properties_view() = default;

   template<typename Type>
   auto value(const std::string_view name, const Type default_value) const -> Type
   {
      auto it =
         std::find_if(_properties.cbegin(), _properties.cend(),
                      [name](const auto& prop) { return prop.name == name; });

      if (it == _properties.cend()) {
         return default_value;
      }

      using Var_type = Material_var<Type>;

      if (!std::holds_alternative<Var_type>(it->value)) {
         throw std::runtime_error{
            "Material property has mismatched type with constant buffer!"};
      }

      const auto& var = std::get<Var_type>(it->value);

      using std::clamp;

      return apply_op(clamp(var.value, var.min, var.max), var.op);
   }

private:
   const std::vector<Material_property>& _properties;
};

void fill_terrain_transforms(material::Constant_buffer_builder& cb,
                             const Material_properties_view& props)
{
   for (std::size_t i = 0; i < terrain_texture_count; ++i) {
      cb.set(fmt::format("texture_transforms[{}]"sv, i * 2),
             props.value<glm::vec3>(fmt::format("TextureTransformsX{}"sv, i),
                                    {1.0f / 16.0f, 0.0f, 0.0f}));
      cb.set(fmt::format("texture_transforms[{}]"sv, i * 2 + 1),
             props.value<glm::vec3>(fmt::format("TextureTransformsY{}"sv, i),
                                    {0.0f, 0.0f, 1.0f / 16.0f}));
   }
}

auto create_pbr_constant_buffer(const Material_properties_view& props)
   -> std::vector<std::byte>
{
   material::Constant_buffer_builder cb{R"(
   float3 base_color;
   float base_metallicness;
   float base_roughness;
   float ao_strength;
   float emissive_power;)"sv};

   cb.set("base_color"sv, props.value<glm::vec3>("BaseColor"sv, {1.0f, 1.0f, 1.0f}));
   cb.set("base_metallicness"sv, props.value<float>("Metallicness"sv, 1.0f));
   cb.set("base_roughness"sv, props.value<float>("Roughness"sv, 1.0f));
   cb.set("ao_strength"sv, props.value<float>("AOStrength"sv, 1.0f));
   cb.set("emissive_power"sv, props.value<float>("EmissivePower"sv, 1.0f));

   return cb.complete();
}

auto create_pbr_terrain_constant_buffer(const Material_properties_view& props)
   -> std::vector<std::byte>
{
   material::Constant_buffer_builder cb{R"(
   float3 base_color;
   float base_metallicness;
   float base_roughness;

   float3 texture_transforms[32];

   float texture_height_scales[16];)"sv};

   cb.set("base_color"sv, props.value<glm::vec3>("BaseColor"sv, {1.0f, 1.0f, 1.0f}));
   cb.set("base_metallicness"sv, props.value<float>("BaseMetallicness"sv, 1.0f));
   cb.set("base_roughness"sv, props.value<float>("BaseRoughness"sv, 1.0f));

   fill_terrain_transforms(cb, props);

   for (std::size_t i = 0; i < terrain_texture_count; ++i) {
      cb.set(fmt::format("texture_height_scales[{}]"sv, i),
             props.value<float>(fmt::format("HeightScale{}"sv, i), 0.025f));
   }

   return cb.complete();
}

auto create_normal_ext_constant_buffer(const Material_properties_view& props)
   -> std::vector<std::byte>
{
   material::Constant_buffer_builder cb{R"(
   float3 base_diffuse_color;
   float  gloss_map_weight;
   float3 base_specular_color;
   float  specular_exponent;
   float  height_scale;
   bool   use_detail_textures;
   float  detail_texture_scale;
   bool   use_overlay_textures;
   float  overlay_texture_scale;
   bool   use_ao_texture;
   bool   use_emissive_texture;
   float  emissive_texture_scale;
   float  emissive_power;
   bool   use_env_map;
   float  env_map_vis;
   float  dynamic_normal_sign;)"sv};

   cb.set("base_diffuse_color"sv,
          props.value<glm::vec3>("DiffuseColor"sv, {1.0f, 1.0f, 1.0f}));

   cb.set("gloss_map_weight"sv, props.value<float>("GlossMapWeight"sv, 1.0f));

   cb.set("base_specular_color"sv,
          props.value<glm::vec3>("SpecularColor"sv, {1.0f, 1.0f, 1.0f}));

   cb.set("specular_exponent"sv, props.value<float>("SpecularExponent"sv, 64.0f));

   cb.set("height_scale"sv, props.value<float>("HeightScale"sv, 0.1f));

   cb.set("use_detail_textures"sv, props.value<bool>("UseDetailMaps"sv, false));

   cb.set("detail_texture_scale"sv, props.value<float>("DetailTextureScale"sv, 1.0f));

   cb.set("use_overlay_textures"sv, props.value<bool>("UseOverlayMaps"sv, false));

   cb.set("overlay_texture_scale"sv, props.value<float>("OverlayTextureScale"sv, 1.0f));

   cb.set("use_ao_texture"sv, props.value<bool>("UseAOMap"sv, false));

   cb.set("use_emissive_texture"sv, props.value<bool>("UseEmissiveMap"sv, false));

   cb.set("emissive_texture_scale"sv,
          props.value<float>("EmissiveTextureScale"sv, 1.0f));

   cb.set("emissive_power"sv, props.value<float>("EmissivePower"sv, 1.0f));

   cb.set("use_env_map"sv, props.value<bool>("UseEnvMap"sv, false));

   cb.set("env_map_vis"sv, props.value<float>("EnvMapVisibility"sv, 1.0f));

   cb.set("dynamic_normal_sign"sv, props.value<float>("DynamicNormalSign"sv, 1.0f));

   return cb.complete();
}

auto create_normal_ext_terrain_constant_buffer(const Material_properties_view& props)
   -> std::vector<std::byte>
{
   material::Constant_buffer_builder cb{R"(   
   float3 diffuse_color;
   float3 specular_color;
   bool use_envmap;

   float3 texture_transforms[32];

   float2 texture_vars[16];)"sv};

   cb.set("diffuse_color"sv,
          props.value<glm::vec3>("DiffuseColor"sv, {1.0f, 1.0f, 1.0f}));
   cb.set("specular_color"sv,
          props.value<glm::vec3>("SpecularColor"sv, {1.0f, 1.0f, 1.0f}));
   cb.set("use_envmap"sv, props.value<bool>("UseEnvmap"sv, false));

   fill_terrain_transforms(cb, props);

   for (std::size_t i = 0; i < terrain_texture_count; ++i) {
      cb.set(fmt::format("texture_vars[{}]"sv, i),
             glm::vec2{props.value<float>(fmt::format("HeightScale{}"sv, i), 0.05f),
                       props.value<float>(fmt::format("SpecularExponent{}"sv, i), 64.0f)});
   }

   return cb.complete();
}

auto create_skybox_constant_buffer(const Material_properties_view& props)
   -> std::vector<std::byte>
{
   material::Constant_buffer_builder cb{R"(
   float emissive_power;)"sv};

   cb.set("emissive_power"sv, props.value<float>("EmissivePower"sv, 1.0f));

   return cb.complete();
}

auto create_basic_unlit_buffer(const Material_properties_view& props)
   -> std::vector<std::byte>
{
   material::Constant_buffer_builder cb{R"(
   bool use_emissive_map;
   float emissive_power;)"sv};

   cb.set("use_emissive_map"sv, props.value<bool>("UseEmissiveMap"sv, false));
   cb.set("emissive_power"sv, props.value<float>("EmissivePower"sv, 1.0f));

   return cb.complete();
}

auto create_static_water_buffer(const Material_properties_view& props)
   -> std::vector<std::byte>
{
   material::Constant_buffer_builder cb{R"(   
   float3 refraction_color;
   float  refraction_scale;
   float3 reflection_color;
   float  small_bump_scale;
   float2 small_scroll;
   float2 medium_scroll;
   float2 large_scroll;
   float  medium_bump_scale;
   float  large_bump_scale;
   float  fresnel_min;
   float  fresnel_max;
   float  specular_exponent_dir_lights;
   float  specular_strength_dir_lights;
   float3 back_refraction_color;
   float  specular_exponent;)"sv};

   cb.set("refraction_color"sv,
          props.value<glm::vec3>("RefractionColor"sv, {0.25f, 0.50f, 0.75f}));
   cb.set("refraction_scale"sv, props.value<float>("RefractionScale"sv, 1.333f));
   cb.set("reflection_color"sv,
          props.value<glm::vec3>("ReflectionColor"sv, {1.0f, 1.0f, 1.0f}));
   cb.set("small_bump_scale"sv, props.value<float>("SmallBumpScale"sv, 1.0f));
   cb.set("small_scroll"sv, props.value<glm::vec2>("SmallScroll"sv, {0.2f, 0.2f}));
   cb.set("medium_scroll"sv, props.value<glm::vec2>("MediumScroll"sv, {0.2f, 0.2f}));
   cb.set("large_scroll"sv, props.value<glm::vec2>("LargeScroll"sv, {0.2f, 0.2f}));
   cb.set("medium_bump_scale"sv, props.value<float>("MediumBumpScale"sv, 1.0f));
   cb.set("large_bump_scale"sv, props.value<float>("LargeBumpScale"sv, 1.0f));
   cb.set("fresnel_min"sv,
          props.value<glm::vec2>("FresnelMinMax"sv, {0.0f, 1.0f}).x);
   cb.set("fresnel_max"sv,
          props.value<glm::vec2>("FresnelMinMax"sv, {0.0f, 1.0f}).y);
   cb.set("specular_exponent_dir_lights"sv,
          props.value<float>("SpecularExponentDirLights"sv, 128.0f));
   cb.set("specular_strength_dir_lights"sv,
          props.value<float>("SpecularStrengthDirLights"sv, 1.0f));
   cb.set("back_refraction_color"sv,
          props.value<glm::vec3>("BackRefractionColor"sv, {0.25f, 0.50f, 0.75f}));
   cb.set("specular_exponent"sv, props.value<float>("SpecularExponent"sv, 64.0f));

   return cb.complete();
}

auto create_particle_ext_buffer(const Material_properties_view& props)
   -> std::vector<std::byte>
{
   material::Constant_buffer_builder cb{R"(
   bool   use_aniso_wrap_sampler;
   float  brightness_scale;)"sv};

   cb.set("use_aniso_wrap_sampler"sv,
          props.value<bool>("UseAnisotropicFiltering"sv, false));
   cb.set("brightness_scale"sv, props.value<float>("EmissivePower"sv, 1.0f));

   return cb.complete();
}

}

auto create_constant_buffer(ID3D11Device5& device, const std::string_view cb_name,
                            const std::vector<Material_property>& properties)
   -> Com_ptr<ID3D11Buffer>
{
   if (cb_name == "none"sv) return nullptr;

   using core::create_immutable_constant_buffer;

   if (Material_properties_view properties_view{properties}; cb_name == "pbr"sv) {
      const auto buffer = create_pbr_constant_buffer(properties_view);

      return create_immutable_constant_buffer(device, std::span{buffer});
   }
   else if (cb_name == "pbr_terrain"sv) {
      const auto buffer = create_pbr_terrain_constant_buffer(properties_view);

      return create_immutable_constant_buffer(device, std::span{buffer});
   }
   else if (cb_name == "normal_ext"sv) {
      const auto buffer = create_normal_ext_constant_buffer(properties_view);

      return create_immutable_constant_buffer(device, std::span{buffer});
   }
   else if (cb_name == "normal_ext_terrain"sv) {
      const auto buffer = create_normal_ext_terrain_constant_buffer(properties_view);

      return create_immutable_constant_buffer(device, std::span{buffer});
   }
   else if (cb_name == "skybox"sv) {
      const auto buffer = create_skybox_constant_buffer(properties_view);

      return create_immutable_constant_buffer(device, std::span{buffer});
   }
   else if (cb_name == "basic_unlit"sv) {
      const auto buffer = create_basic_unlit_buffer(properties_view);

      return create_immutable_constant_buffer(device, std::span{buffer});
   }
   else if (cb_name == "static_water"sv) {
      const auto buffer = create_static_water_buffer(properties_view);

      return create_immutable_constant_buffer(device, std::span{buffer});
   }
   else if (cb_name == "particle_ext"sv) {
      const auto buffer = create_particle_ext_buffer(properties_view);

      return create_immutable_constant_buffer(device, std::span{buffer});
   }

   throw compose_exception<std::runtime_error>("Unknown material constant buffer name "sv,
                                               std::quoted(cb_name), "."sv);
}
}
