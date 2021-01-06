
fail_safe_texture_index = 0

function make_constant_buffer(props)
   local cb = constant_buffer_builder.new([[
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
      float  dynamic_normal_sign;
   ]])

   cb:set("base_diffuse_color",
          props:get_float3("DiffuseColor", float3.new(1.0, 1.0, 1.0)))
   cb:set("gloss_map_weight", props:get_float("GlossMapWeight", 1.0))
   cb:set("base_specular_color",
          props:get_float3("SpecularColor", float3.new(1.0, 1.0, 1.0)))
   cb:set("specular_exponent", props:get_float("SpecularExponent", 64.0))
   cb:set("height_scale", props:get_float("HeightScale", 0.1))
   cb:set("use_detail_textures", props:get_bool("UseDetailMaps", false))
   cb:set("detail_texture_scale", props:get_float("DetailTextureScale", 1.0))
   cb:set("use_overlay_textures", props:get_bool("UseOverlayMaps", false))
   cb:set("overlay_texture_scale", props:get_float("OverlayTextureScale", 1.0))
   cb:set("use_ao_texture", props:get_bool("UseAOMap", false))
   cb:set("use_emissive_texture", props:get_bool("UseEmissiveMap", false))
   cb:set("emissive_texture_scale",
          props:get_float("EmissiveTextureScale", 1.0))
   cb:set("emissive_power", props:get_float("EmissivePower", 1.0))
   cb:set("use_env_map", props:get_bool("UseEnvMap", false))
   cb:set("env_map_vis", props:get_float("EnvMapVisibility", 1.0))
   cb:set("dynamic_normal_sign", props:get_float("DynamicNormalSign", 1.0))

   return cb:complete()
end

function fill_resource_vec(props, resource_props, resources)

   resources:add(resource_props["DiffuseMap"] or "$grey")
   resources:add(resource_props["NormalMap"] or "$null_normalmap")
   resources:add(resource_props["HeightMap"] or "")
   resources:add(resource_props["DetailMap"] or "$null_detailmap")
   resources:add(resource_props["DetailNormalMap"] or "$null_normalmap")
   resources:add(resource_props["EmissiveMap"] or "")
   resources:add(resource_props["OverlayDiffuseMap"] or "$grey")
   resources:add(resource_props["OverlayNormalMap"] or "$null_normalmap")
   resources:add(resource_props["AOMap"] or "$null_ao")
   resources:add(resource_props["EnvMap"] or "")

end
