
local terrain_texture_count <const> = 16

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
