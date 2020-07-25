
#include "material_constant_buffers.hpp"
#include "compose_exception.hpp"
#include "d3d11_helpers.hpp"

#include <iomanip>

using namespace std::literals;

namespace sp::core {

namespace {

struct PBR_cb {
   glm::vec3 base_color = {1.0f, 1.0f, 1.0f};
   float base_metallicness = 1.0f;
   float base_roughness = 1.0f;
   float ao_strength = 1.0f;
   float emissive_power = 1.0f;
   std::uint32_t _padding{};
};

static_assert(sizeof(PBR_cb) == 32);

struct Terrain_texture_transform {
   glm::vec3 x = {1.0f / 16.0f, 0.0f, 0.0f};
   std::uint32_t _padding0{};
   glm::vec3 y = {0.0f, 0.0f, 1.0f / 16.0f};
   std::uint32_t _padding1{};
};

static_assert(sizeof(Terrain_texture_transform) == 32);

struct PBR_terrain_cb {
   glm::vec3 base_color = {1.0f, 1.0f, 1.0f};
   float base_metallicness = 1.0f;

   float base_roughness = 1.0f;
   std::array<std::uint32_t, 3> padding{};

   std::array<Terrain_texture_transform, 16> texture_transforms{};

   std::array<glm::vec4, 16> texture_height_scales{};
};

static_assert(sizeof(PBR_terrain_cb) == 800);

struct Normal_ext_cb {
   glm::vec3 diffuse_color = {1.0f, 1.0f, 1.0f};
   float gloss_map_weight = 1.0f;

   glm::vec3 specular_color = {1.0f, 1.0f, 1.0f};
   float specular_exponent = 64.0f;

   float height_scale = 0.1f;

   std::uint32_t use_detail_textures = false;
   float detail_texture_scale = 1.0f;

   std::uint32_t use_overlay_textures = false;
   float overlay_texture_scale = 1.0f;

   std::uint32_t use_ao_texture = false;

   std::uint32_t use_emissive_texture = false;
   float emissive_texture_scale = 1.0f;
   float emissive_power;

   std::uint32_t use_env_map = false;
   float env_map_vis = 1.0f;

   float dynamic_normal_sign = 1.0f;
};

static_assert(sizeof(Normal_ext_cb) == 80);

struct Normal_ext_texture_vars {
   float height_scale = 0.05f;
   float specular_exponent = 64.0f;
   std::array<std::uint32_t, 2> padding{};
};

static_assert(sizeof(Normal_ext_texture_vars) == 16);

struct Normal_ext_terrain {
   glm::vec3 diffuse_color = {1.0f, 1.0f, 1.0f};
   std::uint32_t _padding{};
   glm::vec3 specular_color = {1.0f, 1.0f, 1.0f};
   std::uint32_t use_envmap = false;

   std::array<Terrain_texture_transform, 16> texture_transforms{};

   std::array<Normal_ext_texture_vars, 16> texture_vars{};
};

static_assert(sizeof(Normal_ext_terrain) == 800);

struct Skybox_cb {
   float emissive_power = 1.0f;
   std::array<std::uint32_t, 3> _padding{};
};

static_assert(sizeof(Skybox_cb) == 16);

struct Basic_unlit_cb {
   std::uint32_t use_emissive_map = false;
   float emissive_power = 1.0f;
   std::array<std::uint32_t, 2> _padding{};
};

static_assert(sizeof(Basic_unlit_cb) == 16);

struct Static_water_cb {
   glm::vec3 refraction_color = {0.25f, 0.50f, 0.75f};
   float refraction_scale = 1.333f;
   glm::vec3 reflection_color = {0.25f, 0.50f, 0.75f};
   float small_bump_scale = 1.0f;
   glm::vec2 small_scroll = {0.2f, 0.2f};
   glm::vec2 medium_scroll = {0.2f, 0.2f};
   glm::vec2 large_scroll = {0.2f, 0.2f};
   float medium_bump_scale = 1.0f;
   float large_bump_scale = 1.0f;
   glm::vec2 fresnel_min_max;
   float specular_exponent_dir_lights = 64.0f;
   float specular_strength_dir_lights = 1.0f;
   glm::vec3 back_refraction_color = {0.25f, 0.50f, 0.75f};
   float specular_exponent = 64.0f;
};

static_assert(sizeof(Static_water_cb) == 96);

struct Particle_ext_cb {
   std::uint32_t use_aniso_wrap_sampler = false;
   float emissive_power = 1.0f;
   std::array<std::uint32_t, 2> _padding{};
};

static_assert(sizeof(Particle_ext_cb) == 16);

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

auto create_terrain_transforms(const Material_properties_view& props)
   -> std::array<Terrain_texture_transform, 16>
{
   std::array<Terrain_texture_transform, 16> transforms{};

   for (auto i = 0; i < transforms.size(); ++i) {
      transforms[i].x =
         props.value<glm::vec3>("TextureTransformsX"s + std::to_string(i),
                                {1.0f / 16.0f, 0.0f, 0.0f});
      transforms[i].y =
         props.value<glm::vec3>("TextureTransformsY"s + std::to_string(i),
                                {0.0f, 0.0f, 1.0f / 16.0f});
   }

   return transforms;
}

auto create_pbr_constant_buffer(const Material_properties_view& props) -> PBR_cb
{
   PBR_cb cb{};

   cb.base_color = props.value<glm::vec3>("BaseColor"sv, cb.base_color);
   cb.base_metallicness = props.value<float>("Metallicness"sv, cb.base_metallicness);
   cb.base_roughness = props.value<float>("Roughness"sv, cb.base_roughness);
   cb.ao_strength = props.value<float>("AOStrength"sv, cb.ao_strength);
   cb.emissive_power = props.value<float>("EmissivePower"sv, cb.emissive_power);

   return cb;
}

auto create_pbr_terrain_constant_buffer(const Material_properties_view& props) -> PBR_terrain_cb
{
   PBR_terrain_cb cb{};

   cb.base_color = props.value<glm::vec3>("BaseColor"sv, cb.base_color);
   cb.base_metallicness =
      props.value<float>("BaseMetallicness"sv, cb.base_metallicness);
   cb.base_roughness = props.value<float>("BaseRoughness"sv, cb.base_roughness);

   cb.texture_transforms = create_terrain_transforms(props);

   for (auto i = 0; i < cb.texture_height_scales.size(); ++i) {
      cb.texture_height_scales[i].x =
         props.value<float>("HeightScale"s + std::to_string(i), 0.025f);
   }

   return cb;
}

auto create_normal_ext_constant_buffer(const Material_properties_view& props) -> Normal_ext_cb
{
   Normal_ext_cb cb{};

   cb.diffuse_color = props.value<glm::vec3>("DiffuseColor"sv, cb.diffuse_color);
   cb.gloss_map_weight = props.value<float>("GlossMapWeight"sv, cb.gloss_map_weight);
   cb.specular_color = props.value<glm::vec3>("SpecularColor"sv, cb.specular_color);
   cb.specular_exponent =
      props.value<float>("SpecularExponent"sv, cb.specular_exponent);
   cb.height_scale = props.value<float>("HeightScale"sv, cb.height_scale);
   cb.use_detail_textures =
      props.value<bool>("UseDetailMaps"sv, cb.use_detail_textures);
   cb.detail_texture_scale =
      props.value<float>("DetailTextureScale"sv, cb.detail_texture_scale);
   cb.use_overlay_textures =
      props.value<bool>("UseOverlayMaps"sv, cb.use_overlay_textures);
   cb.overlay_texture_scale =
      props.value<float>("OverlayTextureScale"sv, cb.overlay_texture_scale);
   cb.use_ao_texture = props.value<bool>("UseAOMap"sv, cb.use_ao_texture);
   cb.use_emissive_texture =
      props.value<bool>("UseEmissiveMap"sv, cb.use_emissive_texture);
   cb.emissive_texture_scale =
      props.value<float>("EmissiveTextureScale"sv, cb.emissive_texture_scale);
   cb.emissive_power = props.value<float>("EmissivePower"sv, cb.emissive_power);
   cb.use_env_map = props.value<bool>("UseEnvMap"sv, cb.use_env_map);
   cb.env_map_vis = props.value<float>("EnvMapVisibility"sv, cb.env_map_vis);
   cb.dynamic_normal_sign =
      props.value<float>("DynamicNormalSign"sv, cb.dynamic_normal_sign);

   return cb;
}

auto create_normal_ext_terrain_constant_buffer(const Material_properties_view& props)
   -> Normal_ext_terrain
{
   Normal_ext_terrain cb{};

   cb.diffuse_color = props.value<glm::vec3>("DiffuseColor"sv, cb.diffuse_color);
   cb.specular_color = props.value<glm::vec3>("SpecularColor"sv, cb.specular_color);
   cb.use_envmap = props.value<bool>("UseEnvmap"sv, cb.use_envmap);

   cb.texture_transforms = create_terrain_transforms(props);

   for (auto i = 0; i < cb.texture_vars.size(); ++i) {
      cb.texture_vars[i].height_scale =
         props.value<float>("HeightScale"s + std::to_string(i), 0.025f);
      cb.texture_vars[i].specular_exponent =
         props.value<float>("SpecularExponent"s + std::to_string(i), 64.0f);
   }

   return cb;
}

auto create_skybox_constant_buffer(const Material_properties_view& props) -> Skybox_cb
{
   Skybox_cb cb{};

   cb.emissive_power = props.value<float>("EmissivePower"sv, cb.emissive_power);

   return cb;
}

auto create_basic_unlit_buffer(const Material_properties_view& props) -> Basic_unlit_cb
{
   Basic_unlit_cb cb{};

   cb.use_emissive_map = props.value<bool>("UseEmissiveMap"sv, cb.use_emissive_map);
   cb.emissive_power = props.value<float>("EmissivePower"sv, cb.emissive_power);

   return cb;
}

auto create_static_water_buffer(const Material_properties_view& props) -> Static_water_cb
{
   Static_water_cb cb{};

   cb.refraction_color = props.value("RefractionColor"sv, cb.refraction_color);
   cb.refraction_scale = props.value("RefractionScale"sv, cb.refraction_scale);
   cb.reflection_color = props.value("ReflectionColor"sv, cb.reflection_color);
   cb.small_bump_scale = props.value("SmallBumpScale"sv, cb.small_bump_scale);
   cb.small_scroll = props.value("SmallScroll"sv, cb.small_scroll);
   cb.medium_scroll = props.value("MediumScroll"sv, cb.medium_scroll);
   cb.large_scroll = props.value("LargeScroll"sv, cb.large_scroll);
   cb.medium_bump_scale = props.value("MediumBumpScale"sv, cb.medium_bump_scale);
   cb.large_bump_scale = props.value("LargeBumpScale"sv, cb.large_bump_scale);
   cb.fresnel_min_max = props.value("FresnelMinMax"sv, cb.fresnel_min_max);
   cb.specular_exponent_dir_lights =
      props.value("SpecularExponentDirLights"sv, cb.specular_exponent_dir_lights);
   cb.specular_strength_dir_lights =
      props.value("SpecularStrengthDirLights"sv, cb.specular_strength_dir_lights);
   cb.back_refraction_color =
      props.value("BackRefractionColor"sv, cb.back_refraction_color);
   cb.specular_exponent = props.value("SpecularExponent"sv, cb.specular_exponent);

   return cb;
}

auto create_particle_ext_buffer(const Material_properties_view& props) -> Particle_ext_cb
{
   Particle_ext_cb cb{};

   cb.use_aniso_wrap_sampler =
      props.value<bool>("UseAnisotropicFiltering"sv, cb.use_aniso_wrap_sampler);
   cb.emissive_power = props.value<float>("EmissivePower"sv, cb.emissive_power);

   return cb;
}

}

auto create_material_constant_buffer(ID3D11Device5& device,
                                     const std::string_view cb_name,
                                     const std::vector<Material_property>& properties)
   -> Com_ptr<ID3D11Buffer>
{
   if (Material_properties_view properties_view{properties}; cb_name == "pbr"sv) {
      return create_immutable_constant_buffer(device, create_pbr_constant_buffer(
                                                         properties_view));
   }
   else if (cb_name == "pbr_terrain"sv) {
      return create_immutable_constant_buffer(device, create_pbr_terrain_constant_buffer(
                                                         properties_view));
   }
   else if (cb_name == "normal_ext"sv) {
      return create_immutable_constant_buffer(device, create_normal_ext_constant_buffer(
                                                         properties_view));
   }
   else if (cb_name == "normal_ext_terrain"sv) {
      return create_immutable_constant_buffer(device, create_normal_ext_terrain_constant_buffer(
                                                         properties_view));
   }
   else if (cb_name == "skybox"sv) {
      return create_immutable_constant_buffer(device, create_skybox_constant_buffer(
                                                         properties_view));
   }
   else if (cb_name == "basic_unlit"sv) {
      return create_immutable_constant_buffer(device, create_basic_unlit_buffer(
                                                         properties_view));
   }
   else if (cb_name == "static_water"sv) {
      return create_immutable_constant_buffer(device, create_static_water_buffer(
                                                         properties_view));
   }
   else if (cb_name == "particle_ext"sv) {
      return create_immutable_constant_buffer(device, create_particle_ext_buffer(
                                                         properties_view));
   }

   throw compose_exception<std::runtime_error>("Unknown material constant buffer name "sv,
                                               std::quoted(cb_name), "."sv);
}
}
