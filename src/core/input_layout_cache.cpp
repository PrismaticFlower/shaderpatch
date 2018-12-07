
#include "input_layout_cache.hpp"

namespace sp::core {

Input_layout_cache::Input_layout_cache(Com_ptr<ID3D11Device1> device) noexcept
   : _device{std::move(device)}
{
}

auto Input_layout_cache::try_emplace(const gsl::span<const D3D11_INPUT_ELEMENT_DESC> layout,
                                     const gsl::span<const std::byte> bytecode,
                                     const bool compressed) noexcept -> Game_input_layout*
{
   if (const auto it = _input_layouts.find(layout); it != _input_layouts.cend()) {
      return &it->second;
   }

   Com_ptr<ID3D11InputLayout> input_layout;

   if (const auto result =
          _device->CreateInputLayout(layout.data(), layout.size(),
                                     bytecode.data(), bytecode.size(),
                                     input_layout.clear_and_assign());
       FAILED(result)) {
      return nullptr;
   }

   return &(_input_layouts[make_key(layout)] = {std::move(input_layout), compressed});
}

auto Input_layout_cache::game_lookup(const gsl::span<const D3D11_INPUT_ELEMENT_DESC> layout) const
   noexcept -> const Game_input_layout*
{
   if (const auto it = _input_layouts.find(layout); it != _input_layouts.cend()) {
      return &it->second;
   }

   return nullptr;
}

auto Input_layout_cache::lookup(const gsl::span<const D3D11_INPUT_ELEMENT_DESC> layout) const
   noexcept -> ID3D11InputLayout*
{

   if (const auto it = _input_layouts.find(layout); it != _input_layouts.cend()) {
      return it->second.layout.get();
   }

   return nullptr;
}

auto Input_layout_cache::make_key(const gsl::span<const D3D11_INPUT_ELEMENT_DESC> layout) noexcept
   -> Key
{
   Key key;
   key.reserve(layout.size());

   for (const auto& elem : layout) {
      key.emplace_back(
         std::make_tuple(std::string{elem.SemanticName}, elem.SemanticIndex,
                         elem.Format, elem.InputSlot, elem.AlignedByteOffset,
                         elem.InputSlotClass, elem.InstanceDataStepRate));
   }

   return key;
}

bool Input_layout_cache::Comparator::operator()(const Key& left, const Key& right) const noexcept
{
   return left < right;
}

bool Input_layout_cache::Comparator::operator()(
   const gsl::span<const D3D11_INPUT_ELEMENT_DESC> left, const Key& right) const noexcept
{
   return std::lexicographical_compare(left.cbegin(), left.cend(),
                                       right.cbegin(), right.cend(),
                                       [](const auto& l, const auto& r) {
                                          return compare_elem_desc(l, r);
                                       });
}

bool Input_layout_cache::Comparator::operator()(
   const Key& left, const gsl::span<const D3D11_INPUT_ELEMENT_DESC> right) const noexcept
{
   return std::lexicographical_compare(left.cbegin(), left.cend(),
                                       right.cbegin(), right.cend(),
                                       [](const auto& l, const auto& r) {
                                          return compare_elem_desc(l, r);
                                       });
}

}
