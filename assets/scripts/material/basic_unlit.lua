
function make_constant_buffer(props)
   local cb = constant_buffer_builder.new([[
      bool use_emissive_map;
      float emissive_power;
   ]])


   cb:set("use_emissive_map", props:get_bool("UseEmissiveMap", false))
   cb:set("emissive_power", props:get_float("EmissivePower", 1.0))

   return cb:complete()
end
