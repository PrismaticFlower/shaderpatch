
constant_buffer_bind = constant_buffer_bind_flag.vs | constant_buffer_bind_flag.ps
fail_safe_texture_index = 1

local terrain_texture_count <const> = 16
local displacement_modes <const> = { none = 0, parallax_offset = 1, occlusion_mapping = 2 }
local blend_modes <const> = { height = 0, basic = 1 }

function make_constant_buffer(props)
   local cb = constant_buffer_builder.new([[
      float3 base_color;
      float base_metallicness;
      float base_roughness;

      float3 texture_transforms[32];

      float texture_height_scales[16];
   ]])
   
   cb:set("base_color", props:get_float3("BaseColor", float3.new(1.0, 1.0, 1.0)))
   cb:set("base_metallicness", props:get_float("BaseMetallicness", 1.0))
   cb:set("base_roughness", props:get_float("BaseRoughness", 1.0))
      
   for i=0, (terrain_texture_count - 1) do
      cb:set(string.format("texture_transforms[%i]", i * 2),
             props:get_float3("TextureTransformsX" .. i,
                              float3.new(1.0 / 16.0, 0.0, 0.0)))
      cb:set(string.format("texture_transforms[%i]", i * 2 + 1),
             props:get_float3("TextureTransformsY" .. i,
                              float3.new(0.0, 0.0, 1.0 / 16.0)))
   end

   for i=0, (terrain_texture_count - 1) do
      cb:set(string.format("texture_height_scales[%i]", i),
             props:get_float("HeightScale" .. i, 0.025))
   end

   return cb:complete()
end

function fill_resource_vec(props, resource_props, resources)

   resources:add(resource_props["height_textures"] or "")
   resources:add(resource_props["albedo_ao_textures"] or "")
   resources:add(resource_props["normal_mr_textures"] or "")
   
end

function get_shader_flags(props, flags)

   local displacement_mode = props:get_int("DisplacementMode", 0)

   if displacement_mode == displacement_modes.parallax_offset then
      flags:add("TERRAIN_COMMON_USE_PARALLAX_OFFSET_MAPPING")
   elseif displacement_mode == displacement_modes.occlusion_mapping then
      flags:add("TERRAIN_COMMON_USE_PARALLAX_OCCLUSION_MAPPING")
   end

   local blend_mode = props:get_int("BlendMode", 0)

   if blend_mode == blend_modes.basic then
      flags:add("TERRAIN_COMMON_BASIC_BLENDING")
   end

   if props:get_bool("LowDetail", false) then
      flags:add("TERRAIN_COMMON_LOW_DETAIL")
   end

end
