
constant_buffer_bind = constant_buffer_bind_flag.ps
fail_safe_texture_index = 0

function make_constant_buffer(props)
   local cb = constant_buffer_builder.new([[
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
      float  specular_exponent;
   ]])

   cb:set("refraction_color",
          props:get_float3("RefractionColor", float3.new(0.25, 0.50, 0.75)))
   cb:set("refraction_scale", props:get_float("RefractionScale", 1.333))
   cb:set("reflection_color",
          props:get_float3("ReflectionColor", float3.new(1.0, 1.0, 1.0)))
   cb:set("small_bump_scale", props:get_float("SmallBumpScale", 1.0))
   cb:set("small_scroll", props:get_float2("SmallScroll", float2.new(0.2, 0.2)))
   cb:set("medium_scroll", props:get_float2("MediumScroll", float2.new(0.2, 0.2)))
   cb:set("large_scroll", props:get_float2("LargeScroll", float2.new(0.2, 0.2)))
   cb:set("medium_bump_scale", props:get_float("MediumBumpScale", 1.0))
   cb:set("large_bump_scale", props:get_float("LargeBumpScale", 1.0))
   cb:set("fresnel_min",
          props:get_float2("FresnelMinMax", float2.new(0.0, 1.0)).x)
   cb:set("fresnel_max",
          props:get_float2("FresnelMinMax", float2.new(0.0, 1.0)).y)
   cb:set("specular_exponent_dir_lights",
          props:get_float("SpecularExponentDirLights", 128.0))
   cb:set("specular_strength_dir_lights",
          props:get_float("SpecularStrengthDirLights", 1.0))
   cb:set("back_refraction_color",
          props:get_float3("BackRefractionColor", float3.new(0.25, 0.50, 0.75)))
   cb:set("specular_exponent", props:get_float("SpecularExponent", 64.0))

   return cb:complete()
end

function fill_resource_vec(props, resource_props, resources)

   resources:add(resource_props["NormalMap"] or "$null_normalmap")
   resources:add(resource_props["ReflectionMap"] or "")
   resources:add(resource_props["DepthBuffer"] or "$depth")
   resources:add(resource_props["RefractionMap"] or "$refraction")
   
end