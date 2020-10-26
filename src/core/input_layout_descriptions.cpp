
#include "input_layout_descriptions.hpp"
#include "../logger.hpp"

#include <algorithm>
#include <tuple>

#include <gsl/gsl>

namespace sp::core {

auto Input_layout_descriptions::try_add(const std::span<const Input_layout_element> layout) noexcept
   -> std::uint16_t
{
   if (const auto index = find_layout(layout); index) return *index;

   const auto index = _descriptions.size();

   if (index > std::numeric_limits<std::uint16_t>::max()) {
      log_and_terminate("Too many input layouts!");
   }

   _descriptions.emplace_back(layout.begin(), layout.end());

   return static_cast<std::uint16_t>(index);
}

auto Input_layout_descriptions::operator[](const std::uint16_t index) const noexcept
   -> std::span<const Input_layout_element>
{
   Expects(index < _descriptions.size());

   return _descriptions[index];
}

auto Input_layout_descriptions::find_layout(
   const std::span<const Input_layout_element> layout) const noexcept
   -> std::optional<std::uint16_t>
{
   for (int i = 0; i < _descriptions.size(); ++i) {
      if (std::equal(_descriptions[i].cbegin(), _descriptions[i].cend(),
                     layout.begin(), layout.end())) {
         return static_cast<std::uint16_t>(i);
      }
   }

   return std::nullopt;
}

}
