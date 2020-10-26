
#include "shader_input_layouts.hpp"
#include "../logger.hpp"

#include <algorithm>

#include <comdef.h>

namespace sp::core {

Shader_input_layouts::Shader_input_layouts(std::vector<Shader_input_element> input_signature,
                                           std::vector<std::byte> bytecode) noexcept
   : _input_signature{std::move(input_signature)}, _bytecode{std::move(bytecode)}
{
}

auto Shader_input_layouts::get(ID3D11Device1& device,
                               const Input_layout_descriptions& descriptions,
                               const std::uint16_t index) noexcept -> ID3D11InputLayout&
{
   if (const auto layout = find_layout(index); layout) return *layout;

   const auto& layout_desc = descriptions[index];

   return *_layouts.emplace_back(index, create_layout(device, layout_desc)).second;
}

auto Shader_input_layouts::find_layout(const std::uint16_t index) const noexcept
   -> ID3D11InputLayout*
{
   for (const auto& layout : _layouts) {
      if (layout.first == index) return layout.second.get();
   }

   return nullptr;
}

auto Shader_input_layouts::create_layout(
   ID3D11Device1& device, const gsl::span<const Input_layout_element> layout_desc) noexcept
   -> Com_ptr<ID3D11InputLayout>
{
   std::vector<D3D11_INPUT_ELEMENT_DESC> input_layout;
   input_layout.reserve(_input_signature.size());

   for (const auto& sig_elem : _input_signature) {
      if (const auto it =
             std::find_if(layout_desc.begin(), layout_desc.end(),
                          [&](const Input_layout_element& elem) {
                             const bool match =
                                (elem.semantic_name == sig_elem.semantic_name) &&
                                (elem.semantic_index == sig_elem.semantic_index);

                             if (match && (dxgi_format_to_input_type(elem.format) !=
                                           sig_elem.input_type)) {
                                log_and_terminate(
                                   "Unexpected IA layout format!");
                             }

                             return match;
                          });
          it != layout_desc.end()) {
         input_layout.emplace_back(*it);
      }
      else {
         auto& elem_desc = input_layout.emplace_back();

         elem_desc.SemanticName = sig_elem.semantic_name.data();
         elem_desc.SemanticIndex = sig_elem.semantic_index;
         elem_desc.Format = input_type_to_dxgi_format(sig_elem.input_type);
         elem_desc.InputSlot = throwaway_input_slot;
         elem_desc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
         elem_desc.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
         elem_desc.InstanceDataStepRate = 1u << 31u;
      }
   }

   Com_ptr<ID3D11InputLayout> il;

   if (const auto result =
          device.CreateInputLayout(input_layout.data(), input_layout.size(),
                                   _bytecode.data(), _bytecode.size(),
                                   il.clear_and_assign());
       FAILED(result)) {
      log_and_terminate("Failed to create Direct3D 11 input layout! Reason: ",
                        _com_error{result}.ErrorMessage());
   }

   return il;
}

}
