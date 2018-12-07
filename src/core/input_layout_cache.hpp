#pragma once

#include "com_ptr.hpp"
#include "game_input_layout.hpp"

#include <map>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include <gsl/gsl>

#include <d3d11_1.h>

namespace sp::core {

class Input_layout_cache {
public:
   explicit Input_layout_cache(Com_ptr<ID3D11Device1> device) noexcept;

   Input_layout_cache(const Input_layout_cache&) = default;
   Input_layout_cache& operator=(const Input_layout_cache&) = default;

   Input_layout_cache(Input_layout_cache&&) = default;
   Input_layout_cache& operator=(Input_layout_cache&&) = default;

   ~Input_layout_cache() = default;

   auto try_emplace(const gsl::span<const D3D11_INPUT_ELEMENT_DESC> layout,
                    const gsl::span<const std::byte> bytecode,
                    const bool compressed = false) noexcept -> Game_input_layout*;

   auto game_lookup(const gsl::span<const D3D11_INPUT_ELEMENT_DESC> layout) const
      noexcept -> const Game_input_layout*;

   auto lookup(const gsl::span<const D3D11_INPUT_ELEMENT_DESC> layout) const
      noexcept -> ID3D11InputLayout*;

private:
   using Index =
      std::tuple<std::string, UINT, DXGI_FORMAT, UINT, UINT, D3D11_INPUT_CLASSIFICATION, UINT>;
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
         std::tuple<std::string_view, UINT, DXGI_FORMAT, UINT, UINT, D3D11_INPUT_CLASSIFICATION, UINT>;

      static bool compare_elem_desc(const D3D11_INPUT_ELEMENT_DESC& elem,
                                    const Index& index) noexcept
      {
         return make_inline_index(elem) < index;
      }

      static bool compare_elem_desc(const Index& index,
                                    const D3D11_INPUT_ELEMENT_DESC& elem) noexcept
      {
         return index < make_inline_index(elem);
      }

      static auto make_inline_index(const D3D11_INPUT_ELEMENT_DESC& elem) noexcept
         -> Inline_index
      {
         return std::make_tuple(std::string_view{elem.SemanticName},
                                elem.SemanticIndex, elem.Format, elem.InputSlot,
                                elem.AlignedByteOffset, elem.InputSlotClass,
                                elem.InstanceDataStepRate);
      }
   };

   static auto make_key(const gsl::span<const D3D11_INPUT_ELEMENT_DESC> layout) noexcept
      -> Key;

   const Com_ptr<ID3D11Device1> _device;

   std::map<Key, Game_input_layout, Comparator> _input_layouts;
};

}
