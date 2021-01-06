
fail_safe_texture_index = 0

function make_constant_buffer(props)
   local cb = constant_buffer_builder.new([[
      bool   use_aniso_wrap_sampler;
      float  brightness_scale;
   ]])


   cb:set("use_aniso_wrap_sampler", props:get_bool("UseAnisotropicFiltering", false))
   cb:set("brightness_scale", props:get_float("EmissivePower", 1.0))

   return cb:complete()
end

function fill_resource_vec(props, resource_props, resources)

   resources:add(resource_props["ColorMap"] or "$white")

end
