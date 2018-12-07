#pragma once

#include "com_ptr.hpp"
#include "game_input_layout.hpp"
#include "input_layout_cache.hpp"

#include <map>
#include <tuple>

#include <d3d11_1.h>

namespace sp::core {

class Input_layout_factory {
public:
   explicit Input_layout_factory(Com_ptr<ID3D11Device1> device) noexcept;

   Input_layout_factory(const Input_layout_factory&) = default;
   Input_layout_factory& operator=(const Input_layout_factory&) = default;

   Input_layout_factory(Input_layout_factory&&) = default;
   Input_layout_factory& operator=(Input_layout_factory&&) = default;

   ~Input_layout_factory() = default;

   bool try_add(const gsl::span<const D3D11_INPUT_ELEMENT_DESC> layout,
                const gsl::span<const std::byte> bytecode,
                const bool compressed = false) noexcept;

   auto game_create(const gsl::span<const D3D11_INPUT_ELEMENT_DESC> layout) noexcept
      -> Game_input_layout;

   auto create(const gsl::span<const D3D11_INPUT_ELEMENT_DESC> layout) noexcept
      -> Com_ptr<ID3D11InputLayout>;

private:
   enum class Input_type {
      float1,
      float2,
      float3,
      float4,
      uint1,
      uint2,
      uint3,
      uint4,
      sint1,
      sint2,
      sint3,
      sint4,
      invalid
   };

   struct Index {
      std::string semantic_name;
      UINT semantic_index;
      Input_type type;
      UINT input_slot;
      D3D11_INPUT_CLASSIFICATION input_slot_class;
      UINT instance_data_step_rate;
   };
   using Key = std::vector<Index>;

   struct Comparator {
      using is_transparent = void;

      bool operator()(const Key& left, const Key& right) const noexcept;
      bool operator()(const gsl::span<const D3D11_INPUT_ELEMENT_DESC> left,
                      const Key& right) const noexcept;
      bool operator()(const Key& left,
                      const gsl::span<const D3D11_INPUT_ELEMENT_DESC> right) const
         noexcept;

   private:
      using Inline_index =
         std::tuple<std::string_view, UINT, Input_type, UINT, D3D11_INPUT_CLASSIFICATION, UINT>;

      static bool compare_elem_desc(const Index& left, const Index& right) noexcept
      {
         return make_inline_index(left) < make_inline_index(right);
      }

      static bool compare_elem_desc(const D3D11_INPUT_ELEMENT_DESC& elem,
                                    const Index& index) noexcept
      {
         return make_inline_index(elem) < make_inline_index(index);
      }

      static bool compare_elem_desc(const Index& index,
                                    const D3D11_INPUT_ELEMENT_DESC& elem) noexcept
      {
         return make_inline_index(index) < make_inline_index(elem);
      }

      static auto make_inline_index(const D3D11_INPUT_ELEMENT_DESC& elem) noexcept
         -> Inline_index
      {
         return std::make_tuple(std::string_view{elem.SemanticName}, elem.SemanticIndex,
                                to_input_type(elem.Format), elem.InputSlot,
                                elem.InputSlotClass, elem.InstanceDataStepRate);
      }

      static auto make_inline_index(const Index& index) noexcept -> Inline_index
      {
         return std::make_tuple(std::string_view{index.semantic_name},
                                index.semantic_index, index.type,
                                index.input_slot, index.input_slot_class,
                                index.instance_data_step_rate);
      }
   };

   static auto to_input_type(const DXGI_FORMAT format) noexcept -> Input_type;

   static auto make_key(const gsl::span<const D3D11_INPUT_ELEMENT_DESC> layout) noexcept
      -> Key;

   const Com_ptr<ID3D11Device1> _device;

   Input_layout_cache _layout_cache{_device};
   std::map<Key, std::pair<std::vector<std::byte>, bool>, Comparator> _bytecode_cache;
};

}
