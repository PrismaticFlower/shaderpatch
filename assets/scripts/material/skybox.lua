
function make_constant_buffer(props)
   local cb = constant_buffer_builder.new([[
      float emissive_power;
   ]])


   cb:set("emissive_power", props:get_float("EmissivePower", 1.0))

   return cb:complete()
end
