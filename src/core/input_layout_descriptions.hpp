#pragma once

#include "input_layout_element.hpp"

#include <cstdint>
#include <optional>
#include <unordered_set>
#include <vector>

#include <gsl/gsl>

#include <d3d11_1.h>

namespace sp::core {

class Input_layout_descriptions {
public:
   auto try_add(const gsl::span<const Input_layout_element> layout) noexcept
      -> std::uint16_t;

   auto operator[](const std::uint16_t index) const noexcept
      -> gsl::span<const Input_layout_element>;

private:
   auto find_layout(const gsl::span<const Input_layout_element> layout) const
      noexcept -> std::optional<std::uint16_t>;

   std::vector<std::vector<Input_layout_element>> _descriptions;
};

}
