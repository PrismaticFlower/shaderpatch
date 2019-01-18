
#include "material_shader.hpp"

namespace sp::core {

Material_shader::Material_shader(Com_ptr<ID3D11Device1> device,
                                 const Shader_rendertype& rendertype) noexcept
   : _device{std::move(device)}, _shaders{init_shaders(rendertype)}
{
}

void Material_shader::update(ID3D11DeviceContext1& dc,
                             const Input_layout_descriptions& layout_descriptions,
                             const std::uint16_t layout_index,
                             const std::string& shader_name,
                             const Vertex_shader_flags vertex_shader_flags,
                             const Pixel_shader_flags pixel_shader_flags) noexcept
{
   auto& state = _shaders.at(shader_name);

   auto& vs = state.first.at(vertex_shader_flags);

   auto& input_layout =
      vs.input_layouts.get(*_device, layout_descriptions, layout_index);

   dc.IASetInputLayout(&input_layout);
   dc.VSSetShader(vs.vs.get(), nullptr, 0);

   dc.PSSetShader(state.second.at(pixel_shader_flags).get(), nullptr, 0);
}

auto Material_shader::init_shaders(const Shader_rendertype& rendertype) noexcept -> Shaders
{
   Shaders shaders;

   for (const auto& state : rendertype) {
      shaders[state.first] = init_state(state.second);
   }

   return shaders;
}

auto Material_shader::init_state(const Shader_state& state) noexcept -> State
{
   return {init_vs_entrypoint(state.vs_entrypoint(), state.vs_static_flags()),
           init_ps_entrypoint(state.ps_entrypoint(), state.ps_static_flags())};
}

auto Material_shader::init_vs_entrypoint(const Vertex_shader_entrypoint& vs,
                                         const std::uint16_t static_flags) noexcept
   -> std::unordered_map<Vertex_shader_flags, Material_vertex_shader>
{
   std::unordered_map<Vertex_shader_flags, Material_vertex_shader> shaders;

   vs.copy_all_if(
      static_flags, [&](const Vertex_shader_flags flags,
                        std::tuple<Com_ptr<ID3D11VertexShader>, std::vector<std::byte>,
                                   std::vector<Shader_input_element>>
                           shader_bc_input_sig) noexcept {
         auto& [shader, bytecode, input_sig] = shader_bc_input_sig;

         shaders.emplace(flags,
                         Material_vertex_shader{std::move(shader),
                                                Shader_input_layouts{std::move(input_sig),
                                                                     std::move(bytecode)}});
      });

   return shaders;
}

auto Material_shader::init_ps_entrypoint(const Pixel_shader_entrypoint& ps,
                                         const std::uint16_t static_flags) noexcept
   -> std::unordered_map<Pixel_shader_flags, Com_ptr<ID3D11PixelShader>>
{
   std::unordered_map<Pixel_shader_flags, Com_ptr<ID3D11PixelShader>> variations;

   ps.copy_all_if(
      static_flags, [&variations](const Pixel_shader_flags flags,
                                  Com_ptr<ID3D11PixelShader> shader) noexcept {
         variations[flags] = std::move(shader);
      });

   return variations;
}

}
