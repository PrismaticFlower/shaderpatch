
function make_constant_buffer(props)
   local cb = constant_buffer_builder.new([[
      float3 base_color;
      float base_metallicness;
      float base_roughness;
      float ao_strength;
      float emissive_power;
   ]])

   cb:set("base_color", props:get_float3("BaseColor", float3.new(1.0, 1.0, 1.0)))
   cb:set("base_metallicness", props:get_float("Metallicness", 1.0))
   cb:set("base_roughness", props:get_float("Roughness", 1.0))
   cb:set("ao_strength", props:get_float("AOStrength", 1.0))
   cb:set("emissive_power", props:get_float("EmissivePower", 1.0))

   return cb:complete()
end
