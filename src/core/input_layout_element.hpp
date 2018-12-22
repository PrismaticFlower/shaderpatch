#pragma once

#include <string>
#include <tuple>

#include <d3d11_1.h>

namespace sp::core {

struct Input_layout_element {
   Input_layout_element() = default;

   Input_layout_element(const D3D11_INPUT_ELEMENT_DESC& desc) noexcept
      : semantic_name{desc.SemanticName},
        semantic_index{desc.SemanticIndex},
        format{desc.Format},
        input_slot{desc.InputSlot},
        aligned_byte_offset{desc.AlignedByteOffset},
        input_slot_class{desc.InputSlotClass},
        instance_data_step_rate{desc.InstanceDataStepRate}
   {
   }

   operator const D3D11_INPUT_ELEMENT_DESC() const noexcept
   {
      return {semantic_name.c_str(),
              semantic_index,
              format,
              input_slot,
              aligned_byte_offset,
              input_slot_class,
              instance_data_step_rate};
   }

   std::string semantic_name;
   UINT semantic_index;
   DXGI_FORMAT format;
   UINT input_slot;
   UINT aligned_byte_offset;
   D3D11_INPUT_CLASSIFICATION input_slot_class;
   UINT instance_data_step_rate;
};

inline bool operator==(const Input_layout_element& left,
                       const Input_layout_element& right) noexcept
{
   return std::tie(left.semantic_name, left.semantic_index, left.format,
                   left.input_slot, left.aligned_byte_offset,
                   left.input_slot_class, left.instance_data_step_rate) ==
          std::tie(right.semantic_name, right.semantic_index, right.format,
                   right.input_slot, right.aligned_byte_offset,
                   right.input_slot_class, right.instance_data_step_rate);
}

inline bool operator!=(const Input_layout_element& left,
                       const Input_layout_element& right) noexcept
{
   return std::tie(left.semantic_name, left.semantic_index, left.format,
                   left.input_slot, left.aligned_byte_offset,
                   left.input_slot_class, left.instance_data_step_rate) ==
          std::tie(right.semantic_name, right.semantic_index, right.format,
                   right.input_slot, right.aligned_byte_offset,
                   right.input_slot_class, right.instance_data_step_rate);
}

}
