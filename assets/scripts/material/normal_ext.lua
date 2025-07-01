
constant_buffer_bind = constant_buffer_bind_flag.ps
fail_safe_texture_index = 0

function make_constant_buffer(props, resources_desc_view)
   local cb = constant_buffer_builder.new([[
      float3 base_diffuse_color;
      float  gloss_map_weight;
      float3 base_specular_color;
      float  base_specular_exponent;
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
      float3 interior_spacing;
      uint   interior_hash_seed;
      float2 interior_map_array_size_info;
      bool   interior_randomize_walls;
   ]])

   cb:set("base_diffuse_color",
          props:get_float3("DiffuseColor", float3.new(1.0, 1.0, 1.0)))
   cb:set("gloss_map_weight", props:get_float("GlossMapWeight", 1.0))
   cb:set("base_specular_color",
          props:get_float3("SpecularColor", float3.new(1.0, 1.0, 1.0)))
   cb:set("base_specular_exponent", props:get_float("SpecularExponent", 64.0))
   cb:set("height_scale", props:get_float("HeightScale", 0.1))
   cb:set("use_detail_textures", props:get_bool("UseDetailMaps", false))
   cb:set("detail_texture_scale", props:get_float("DetailTextureScale", 1.0))
   cb:set("use_overlay_textures", props:get_bool("UseOverlayMaps", false))
   cb:set("overlay_texture_scale", props:get_float("OverlayTextureScale", 1.0))
   cb:set("use_ao_texture", props:get_bool("UseAOMap", false))
   cb:set("use_emissive_texture", props:get_bool("UseEmissiveMap", false))
   cb:set("emissive_texture_scale",
          props:get_float("EmissiveTextureScale", 1.0))
   cb:set("emissive_power", math2.exp2(props:get_float("EmissivePower", 0.0)))
   cb:set("use_env_map", props:get_bool("UseEnvMap", false))
   cb:set("env_map_vis", props:get_float("EnvMapVisibility", 1.0))
   cb:set("dynamic_normal_sign", math2.sign(props:get_float("DynamicNormalSign", 1.0)))
   cb:set("interior_spacing", 
          props:get_float3("InteriorRoomSize", float3.new(1.0, 1.0, 1.0)))
   cb:set_uint("interior_hash_seed", props:get_uint("InteriorRandomnessSeed", 0))

   local interior_map_index = 10
   local interior_map_array_size = resources_desc_view.ps:get(interior_map_index).array_size

   cb:set("interior_map_array_size_info", float2.new(1.0 / interior_map_array_size, interior_map_array_size))
   cb:set("interior_randomize_walls", props:get_bool("InteriorRandomizeWalls", true))

   return cb:complete()
end

function fill_resource_vec(props, resource_props, resources)

   resources:add(resource_props["DiffuseMap"] or "$grey")
   resources:add(resource_props["SpecularMap"] or "")
   resources:add(resource_props["NormalMap"] or "$null_normalmap")
   resources:add(resource_props["HeightMap"] or "")
   resources:add(resource_props["DetailMap"] or "$null_detailmap")
   resources:add(resource_props["DetailNormalMap"] or "$null_normalmap")
   resources:add(resource_props["EmissiveMap"] or "")
   resources:add(resource_props["OverlayDiffuseMap"] or "$grey")
   resources:add(resource_props["OverlayNormalMap"] or "$null_normalmap")
   resources:add(resource_props["AOMap"] or "$null_ao")
   resources:add(resource_props["EnvMap"] or "")
   resources:add(resource_props["InteriorMap"] or "")

end

function get_shader_flags(props, flags)

   if props:get_bool("UseSpecularLighting", false) then
      flags:add("NORMAL_EXT_USE_SPECULAR")

      if props:get_bool("UseSpecularMap", false) then
         flags:add("NORMAL_EXT_USE_SPECULAR_MAP")
      end
   
      if props:get_bool("UseTrueGlossMap", false) then
         flags:add("NORMAL_EXT_USE_SPECULAR_TRUE_GLOSS")
      end
   
      if props:get_bool("UseSpecularLightingNormalized", false) then
         flags:add("NORMAL_EXT_USE_SPECULAR_NORMALIZED")
      end
   end

   if props:get_bool("UseParallaxOcclusionMapping", false) then
      flags:add("NORMAL_EXT_USE_PARALLAX_OCCLUSION_MAPPING")
   end

   if props:get_bool("IsDynamic", false) then
      flags:add("NORMAL_EXT_USE_DYNAMIC_TANGENTS")
   end

   if props:get_bool("UseEmissiveMap", false) and props:get_bool("MultiplyEmissiveByVertexColor", false) then
      flags:add("NORMAL_EXT_USE_VERTEX_COLOR_FOR_EMISSIVE")
   end

   if props:get_bool("UseInteriorMapping", false) then
      flags:add("NORMAL_EXT_USE_INTERIOR_MAPPING")
   end
   
end
