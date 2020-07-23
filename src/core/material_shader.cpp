
#include "material_shader.hpp"
#include "../logger.hpp"
#include "../user_config.hpp"

#include <bitset>

using namespace std::literals;

namespace sp::core {

namespace {

template<typename Enum>
auto flags_to_bitstring(const Enum flags) -> std::string
{
   return std::bitset<32>{static_cast<unsigned long>(flags)}.to_string();
}

}

Material_shader::Material_shader(Com_ptr<ID3D11Device1> device,
                                 const Shader_rendertype& rendertype,
                                 std::string name) noexcept
   : _device{std::move(device)}, _shaders{init_shaders(rendertype)}, _name{std::move(name)}
{
}

void Material_shader::update(ID3D11DeviceContext1& dc,
                             const Input_layout_descriptions& layout_descriptions,
                             const std::uint16_t layout_index,
                             const std::string& state_name,
                             const Vertex_shader_flags vertex_shader_flags,
                             const Pixel_shader_flags pixel_shader_flags,
                             const bool oit_active) noexcept
{
   auto& state = get_state(state_name);

   auto& vs = state.get_vs(vertex_shader_flags, state_name, _name);

   auto& input_layout =
      vs.input_layouts.get(*_device, layout_descriptions, layout_index);

   dc.IASetInputLayout(&input_layout);
   dc.VSSetShader(vs.vs.get(), nullptr, 0);

   dc.PSSetShader(&state.get_ps(pixel_shader_flags, oit_active, state_name, _name),
                  nullptr, 0);
}

auto Material_shader::Material_shader_state::get_vs(const Vertex_shader_flags flags,
                                                    const std::string& state_name,
                                                    const std::string& shader_name) noexcept
   -> Material_vertex_shader&
{
   if (auto shader = vertex.find(flags); shader != vertex.cend()) {
      return shader->second;
   }

   log_and_terminate("Failed to find vertex shader for material shader '"sv,
                     shader_name, "' with shader state '"sv, state_name,
                     "'! vertex shader flags: ("sv, flags_to_bitstring(flags),
                     ") '"sv, to_string(flags), "'"sv);
}

auto Material_shader::Material_shader_state::get_ps(const Pixel_shader_flags flags,
                                                    const bool oit_active,
                                                    const std::string& state_name,
                                                    const std::string& shader_name) noexcept
   -> ID3D11PixelShader&
{
   auto& shaders = oit_active ? pixel : pixel_oit;

   if (auto shader = shaders.find(flags); shader != shaders.cend()) {
      return *shader->second;
   }

   log_and_terminate("Failed to find vertex shader for material shader '"sv,
                     shader_name, "' with shader state '"sv, state_name,
                     "'! pixel shader flags: ("sv, flags_to_bitstring(flags),
                     ") '"sv, to_string(flags), "' oit: '"sv,
                     oit_active ? "true"sv : "false"sv, "'"sv);
}

auto Material_shader::get_state(const std::string& state_name) noexcept
   -> Material_shader_state&
{
   if (auto state = _shaders.find(state_name); state != _shaders.cend()) {
      return state->second;
   }

   log_and_terminate("Failed to find shader state '"sv, state_name,
                     "' for material shader '"sv, _name, "'!"sv);
}

auto Material_shader::init_shaders(const Shader_rendertype& rendertype) noexcept -> Shaders
{
   Shaders shaders;

   for (const auto& state : rendertype) {
      shaders[state.first] = init_state(state.second);
   }

   return shaders;
}

auto Material_shader::init_state(const Shader_state& state) noexcept -> Material_shader_state
{
   return {init_vs_entrypoint(state.vertex.entrypoint(), state.vertex.static_flags()),
           init_ps_entrypoint(state.pixel.entrypoint(), state.pixel.static_flags()),
           init_ps_entrypoint(state.pixel_oit.entrypoint(),
                              state.pixel_oit.static_flags())};
}

auto Material_shader::init_vs_entrypoint(const Vertex_shader_entrypoint& vs,
                                         const std::uint16_t static_flags) noexcept
   -> std::unordered_map<Vertex_shader_flags, Material_vertex_shader>
{
   std::unordered_map<Vertex_shader_flags, Material_vertex_shader> shaders;

   vs.copy_all_if(static_flags, [&](const Vertex_shader_flags flags,
                                    std::tuple<Com_ptr<ID3D11VertexShader>,
                                               std::vector<std::byte>, std::vector<Shader_input_element>>
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

   ps.copy_all_if(static_flags, [&variations](const Pixel_shader_flags flags,
                                              Com_ptr<ID3D11PixelShader> shader) noexcept {
      variations[flags] = std::move(shader);
   });

   return variations;
}

}
