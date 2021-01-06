
fail_safe_texture_index = 0

function fill_resource_vec(props, resource_props, resources)

   resources:add(resource_props["atlas"] or "")

end

function fill_vs_resource_vec(props, resource_props, resources)

   resources:add(resource_props["atlas_index"] or "")

end
