
constant_buffer_bind = constant_buffer_bind_flag.ps
fail_safe_texture_index = 0

function make_constant_buffer(props, resources_desc_view)
   local cb = constant_buffer_builder.new([[
      float3 base_diffuse_color;
      float  gloss_map_weight;
      float3 base_specular_color;
      float  specular_exponent;
      bool   use_ao_texture;
      bool   use_emissive_texture;
      float  emissive_texture_scale;
      float  emissive_power;
      bool   use_env_map;
      float  env_map_vis;
      float  dynamic_normal_sign;
      bool   use_outline_light;
      float3 outline_light_color;
      float  outline_light_width; 
      float  outline_light_fade;
   ]])

   cb:set("base_diffuse_color",
          props:get_float3("DiffuseColor", float3.new(1.0, 1.0, 1.0)))
   cb:set("gloss_map_weight", props:get_float("GlossMapWeight", 1.0))
   cb:set("base_specular_color",
          props:get_float3("SpecularColor", float3.new(1.0, 1.0, 1.0)))
   cb:set("specular_exponent", props:get_float("SpecularExponent", 64.0))
   cb:set("use_ao_texture", props:get_bool("UseAOMap", false))
   cb:set("use_emissive_texture", props:get_bool("UseEmissiveMap", false))
   cb:set("emissive_texture_scale",
          props:get_float("EmissiveTextureScale", 1.0))
   cb:set("emissive_power", math2.exp2(props:get_float("EmissivePower", 0.0)))
   cb:set("use_env_map", props:get_bool("UseEnvMap", false))
   cb:set("env_map_vis", props:get_float("EnvMapVisibility", 1.0))
   cb:set("dynamic_normal_sign", math2.sign(props:get_float("DynamicNormalSign", 1.0)))
   cb:set("use_outline_light", props:get_bool("UseOutlineLight", false))
   cb:set("outline_light_color", props:get_float3("OutlineLightColor", float3.new(1.0, 1.0, 1.0)))
   cb:set("outline_light_width", props:get_float("OutlineLightWidth", 0.25))
   cb:set("outline_light_fade", props:get_float("OutlineLightFade", 0.5))

   return cb:complete()
end

function fill_resource_vec(props, resource_props, resources)

   resources:add(resource_props["DiffuseMap"] or "$grey")
   resources:add(resource_props["NormalMap"] or "$null_normalmap")
   resources:add(resource_props["AOMap"] or "$null_ao")
   resources:add(resource_props["EmissiveMap"] or "")
   resources:add(resource_props["EnvMap"] or "")

end

function get_shader_flags(props, flags)

   if props:get_bool("IsDynamic", false) then
      flags:add("NORMAL_BF3_USE_DYNAMIC_TANGENTS")
   end

end
