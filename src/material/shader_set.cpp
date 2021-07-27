
#include "shader_set.hpp"
#include "../logger.hpp"
#include "../user_config.hpp"

#include <bitset>

using namespace std::literals;

namespace sp::material {

namespace {

template<typename Enum>
auto flags_to_bitstring(const Enum flags) -> std::string
{
   return std::bitset<32>{static_cast<unsigned long>(flags)}.to_string();
}

}

Shader_set::Shader_set(Com_ptr<ID3D11Device1> device, shader::Rendertype& rendertype,
                       std::span<const std::string> extra_flags, std::string name) noexcept
   : _device{std::move(device)}, _shaders{init_shaders(rendertype, extra_flags)}, _name{std::move(name)}
{
}

void Shader_set::update(ID3D11DeviceContext1& dc,
                        const core::Input_layout_descriptions& layout_descriptions,
                        const std::uint16_t layout_index, const std::string& state_name,
                        const shader::Vertex_shader_flags vertex_shader_flags,
                        const bool oit_active) noexcept
{
   auto& state = get_state(state_name);

   auto& vs = state.get_vs(vertex_shader_flags, state_name, _name);

   auto& input_layout =
      vs.input_layouts.get(*_device, layout_descriptions, layout_index);

   dc.IASetInputLayout(&input_layout);
   dc.VSSetShader(vs.vs.get(), nullptr, 0);

   dc.PSSetShader((oit_active ? state.pixel_oit : state.pixel).get(), nullptr, 0);
}

auto Shader_set::Material_shader_state::get_vs(const shader::Vertex_shader_flags flags,
                                               const std::string& state_name,
                                               const std::string& shader_name) noexcept
   -> Material_vertex_shader&
{
   if (auto shader = vertex.find(flags); shader != vertex.cend()) {
      return shader->second;
   }

   log_and_terminate("Failed to find vertex shader for material shader '{}' with shader state '{}'! vertex shader flags: ({}) '{}'"sv,
                     shader_name, state_name, flags_to_bitstring(flags),
                     to_string(flags));
}

auto Shader_set::get_state(const std::string& state_name) noexcept
   -> Material_shader_state&
{
   if (auto state = _shaders.find(state_name); state != _shaders.cend()) {
      return state->second;
   }

   log_and_terminate("Failed to find shader state '{}' for material shader '{}'!"sv,
                     state_name, _name);
}

auto Shader_set::init_shaders(shader::Rendertype& rendertype,
                              std::span<const std::string> extra_flags) noexcept -> Shaders
{
   Shaders shaders;

   for (const auto& [name, state] : rendertype) {
      shaders[name] = init_state(*state, extra_flags);
   }

   return shaders;
}

auto Shader_set::init_state(shader::Rendertype_state& state,
                            std::span<const std::string> extra_flags) noexcept
   -> Material_shader_state
{
   return {init_vs_entrypoint(state, extra_flags), state.pixel(extra_flags),
           state.pixel_oit(extra_flags)};
}

auto Shader_set::init_vs_entrypoint(shader::Rendertype_state& state,
                                    std::span<const std::string> extra_flags) noexcept
   -> absl::flat_hash_map<shader::Vertex_shader_flags, Material_vertex_shader>
{
   absl::flat_hash_map<shader::Vertex_shader_flags, Material_vertex_shader> shaders;

   state.vertex_copy_all(
      [&](shader::Vertex_shader_flags flags, Com_ptr<ID3D11VertexShader> shader,
          sp::shader::Bytecode_blob bytecode,
          sp::shader::Vertex_input_layout input_sig) noexcept {
         shaders.emplace(flags,
                         Material_vertex_shader{.vs = std::move(shader),
                                                .input_layouts = {std::move(input_sig),
                                                                  std::move(bytecode)}});
      },
      extra_flags);

   return shaders;
}

}
