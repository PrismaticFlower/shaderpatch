
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
                                 shader::Rendertype& rendertype, std::string name) noexcept
   : _device{std::move(device)}, _shaders{init_shaders(rendertype)}, _name{std::move(name)}
{
}

void Material_shader::update(ID3D11DeviceContext1& dc,
                             const Input_layout_descriptions& layout_descriptions,
                             const std::uint16_t layout_index,
                             const std::string& state_name,
                             const shader::Vertex_shader_flags vertex_shader_flags,
                             const bool oit_active) noexcept
{
   auto& state = get_state(state_name);

   auto& vs = state.get_vs(vertex_shader_flags, state_name, _name);

   auto& input_layout =
      vs.input_layouts.get(*_device, layout_descriptions, layout_index);

   dc.IASetInputLayout(&input_layout);
   dc.VSSetShader(vs.vs.get(), nullptr, 0);

   dc.GSSetShader(state.geometry.get(), nullptr, 0);

   dc.PSSetShader((oit_active ? state.pixel_oit : state.pixel).get(), nullptr, 0);
}

auto Material_shader::Material_shader_state::get_vs(
   const shader::Vertex_shader_flags flags, const std::string& state_name,
   const std::string& shader_name) noexcept -> Material_vertex_shader&
{
   if (auto shader = vertex.find(flags); shader != vertex.cend()) {
      return shader->second;
   }

   log_and_terminate("Failed to find vertex shader for material shader '"sv,
                     shader_name, "' with shader state '"sv, state_name,
                     "'! vertex shader flags: ("sv, flags_to_bitstring(flags),
                     ") '"sv, to_string(flags), "'"sv);
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

auto Material_shader::init_shaders(shader::Rendertype& rendertype) noexcept -> Shaders
{
   Shaders shaders;

   for (const auto& [name, state] : rendertype) {
      shaders[name] = init_state(*state);
   }

   return shaders;
}

auto Material_shader::init_state(shader::Rendertype_state& state) noexcept -> Material_shader_state
{
   return {init_vs_entrypoint(state), state.geometry(), state.pixel(),
           state.pixel_oit()};
}

auto Material_shader::init_vs_entrypoint(shader::Rendertype_state& state) noexcept
   -> std::unordered_map<shader::Vertex_shader_flags, Material_vertex_shader>
{
   std::unordered_map<shader::Vertex_shader_flags, Material_vertex_shader> shaders;

   state.vertex_copy_all([&](shader::Vertex_shader_flags flags,
                             Com_ptr<ID3D11VertexShader> shader,
                             sp::shader::Bytecode_blob bytecode,
                             sp::shader::Vertex_input_layout input_sig) noexcept {
      shaders.emplace(flags,
                      Material_vertex_shader{.vs = std::move(shader),
                                             .input_layouts = {std::move(input_sig),
                                                               std::move(bytecode)}});
   });

   return shaders;
}

}
