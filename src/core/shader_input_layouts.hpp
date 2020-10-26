#pragma once

#include "com_ptr.hpp"
#include "input_layout_descriptions.hpp"
#include "shader_input_element.hpp"

#include <cstddef>
#include <span>
#include <vector>

#include <d3d11_1.h>

namespace sp::core {

class Shader_input_layouts {
public:
   Shader_input_layouts(std::vector<Shader_input_element> input_signature,
                        std::vector<std::byte> bytecode) noexcept;

   Shader_input_layouts(const Shader_input_layouts&) = default;
   Shader_input_layouts(Shader_input_layouts&&) = default;
   ~Shader_input_layouts() = default;

   Shader_input_layouts() = delete;
   Shader_input_layouts& operator=(const Shader_input_layouts&) = delete;
   Shader_input_layouts& operator=(Shader_input_layouts&&) = delete;

   auto get(ID3D11Device1& device, const Input_layout_descriptions& descriptions,
            const std::uint16_t index) noexcept -> ID3D11InputLayout&;

   constexpr static auto throwaway_input_slot = 1;

private:
   auto find_layout(const std::uint16_t index) const noexcept -> ID3D11InputLayout*;

   auto create_layout(ID3D11Device1& device,
                      const std::span<const Input_layout_element> descriptions) noexcept
      -> Com_ptr<ID3D11InputLayout>;

   std::vector<std::pair<std::int32_t, Com_ptr<ID3D11InputLayout>>> _layouts;
   const std::vector<Shader_input_element> _input_signature;
   const std::vector<std::byte> _bytecode;
};

}
