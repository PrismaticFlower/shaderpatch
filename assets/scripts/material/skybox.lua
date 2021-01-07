
constant_buffer_bind = constant_buffer_bind_flag.ps
fail_safe_texture_index = 0

function make_constant_buffer(props)
   local cb = constant_buffer_builder.new([[
      float emissive_power;
   ]])


   cb:set("emissive_power", props:get_float("EmissivePower", 1.0))

   return cb:complete()
end

function fill_resource_vec(props, resource_props, resources)
   
   resources:add(resource_props["Skybox"] or "")
   resources:add(resource_props["SkyboxEmissive"] or "")

end
