
#include "input_layout_factory.hpp"
#include "../logger.hpp"

#include <algorithm>

namespace sp::core {

Input_layout_factory::Input_layout_factory(Com_ptr<ID3D11Device1> device) noexcept
   : _device{std::move(device)}
{
}

bool Input_layout_factory::try_add(const gsl::span<const D3D11_INPUT_ELEMENT_DESC> layout,
                                   const gsl::span<const std::byte> bytecode,
                                   const bool compressed) noexcept
{
   _bytecode_cache[make_key(layout)] = {std::vector(bytecode.cbegin(), bytecode.cend()),
                                        compressed};

   return _layout_cache.try_emplace(layout, bytecode, compressed) != nullptr;
}

auto Input_layout_factory::game_create(const gsl::span<const D3D11_INPUT_ELEMENT_DESC> layout) noexcept
   -> Game_input_layout
{
   if (const auto cached = _layout_cache.game_lookup(layout); cached)
      return *cached;

   if (const auto it = _bytecode_cache.find(layout); it != _bytecode_cache.cend()) {

      if (const auto game_layout =
             _layout_cache.try_emplace(layout, it->second.first, it->second.second);
          game_layout)
         return *game_layout;
   }

   log(Log_level::warning,
       "Attempt to create IA layout with no compatible shader!");

   return {};
}

auto Input_layout_factory::create(const gsl::span<const D3D11_INPUT_ELEMENT_DESC> layout) noexcept
   -> Com_ptr<ID3D11InputLayout>
{
   return game_create(layout).layout;
}

auto Input_layout_factory::to_input_type(const DXGI_FORMAT format) noexcept -> Input_type
{
   switch (format) {
   case DXGI_FORMAT_R32G32B32A32_FLOAT:
   case DXGI_FORMAT_R16G16B16A16_FLOAT:
   case DXGI_FORMAT_R16G16B16A16_UNORM:
   case DXGI_FORMAT_R16G16B16A16_SNORM:
   case DXGI_FORMAT_R10G10B10A2_UNORM:
   case DXGI_FORMAT_R8G8B8A8_UNORM:
   case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
   case DXGI_FORMAT_R8G8B8A8_SNORM:
   case DXGI_FORMAT_B5G5R5A1_UNORM:
   case DXGI_FORMAT_B8G8R8A8_UNORM:
   case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
   case DXGI_FORMAT_B4G4R4A4_UNORM:
      return Input_type::float4;
   case DXGI_FORMAT_R32G32B32_FLOAT:
   case DXGI_FORMAT_R11G11B10_FLOAT:
   case DXGI_FORMAT_B5G6R5_UNORM:
   case DXGI_FORMAT_B8G8R8X8_UNORM:
   case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
      return Input_type::float3;
   case DXGI_FORMAT_R32G32_FLOAT:
   case DXGI_FORMAT_R16G16_FLOAT:
   case DXGI_FORMAT_R16G16_UNORM:
   case DXGI_FORMAT_R16G16_SNORM:
      return Input_type::float2;
   case DXGI_FORMAT_R32_FLOAT:
   case DXGI_FORMAT_R16_FLOAT:
   case DXGI_FORMAT_R16_UNORM:
   case DXGI_FORMAT_R16_SNORM:
   case DXGI_FORMAT_R8_UNORM:
   case DXGI_FORMAT_R8_SNORM:
      return Input_type::float1;
   case DXGI_FORMAT_R32G32B32A32_UINT:
   case DXGI_FORMAT_R16G16B16A16_UINT:
   case DXGI_FORMAT_R10G10B10A2_UINT:
   case DXGI_FORMAT_R8G8B8A8_UINT:
      return Input_type::uint4;
   case DXGI_FORMAT_R32G32B32_UINT:
      return Input_type::uint3;
   case DXGI_FORMAT_R32G32_UINT:
   case DXGI_FORMAT_R16G16_UINT:
      return Input_type::uint2;
   case DXGI_FORMAT_R32_UINT:
   case DXGI_FORMAT_R16_UINT:
   case DXGI_FORMAT_R8_UINT:
      return Input_type::uint1;
   case DXGI_FORMAT_R32G32B32A32_SINT:
   case DXGI_FORMAT_R16G16B16A16_SINT:
   case DXGI_FORMAT_R8G8B8A8_SINT:
      return Input_type::sint4;
   case DXGI_FORMAT_R32G32B32_SINT:
      return Input_type::sint3;
   case DXGI_FORMAT_R32G32_SINT:
   case DXGI_FORMAT_R16G16_SINT:
      return Input_type::sint2;
   case DXGI_FORMAT_R32_SINT:
   case DXGI_FORMAT_R16_SINT:
   case DXGI_FORMAT_R8_SINT:
      return Input_type::sint1;
   default:
      return Input_type::invalid;
   }
}

auto Input_layout_factory::make_key(const gsl::span<const D3D11_INPUT_ELEMENT_DESC> layout) noexcept
   -> Key
{
   Key key;
   key.reserve(layout.size());

   for (const auto& elem : layout) {
      key.push_back({std::string{elem.SemanticName}, elem.SemanticIndex,
                     to_input_type(elem.Format), elem.InputSlot,
                     elem.InputSlotClass, elem.InstanceDataStepRate});
   }

   return key;
}

bool Input_layout_factory::Comparator::operator()(const Key& left,
                                                  const Key& right) const noexcept
{
   return std::lexicographical_compare(left.cbegin(), left.cend(),
                                       right.cbegin(), right.cend(),
                                       [](const auto& l, const auto& r) {
                                          return compare_elem_desc(l, r);
                                       });
}

bool Input_layout_factory::Comparator::operator()(
   const gsl::span<const D3D11_INPUT_ELEMENT_DESC> left, const Key& right) const noexcept
{
   return std::lexicographical_compare(left.cbegin(), left.cend(),
                                       right.cbegin(), right.cend(),
                                       [](const auto& l, const auto& r) {
                                          return compare_elem_desc(l, r);
                                       });
}

bool Input_layout_factory::Comparator::operator()(
   const Key& left, const gsl::span<const D3D11_INPUT_ELEMENT_DESC> right) const noexcept
{
   return std::lexicographical_compare(left.cbegin(), left.cend(),
                                       right.cbegin(), right.cend(),
                                       [](const auto& l, const auto& r) {
                                          return compare_elem_desc(l, r);
                                       });
}

}
