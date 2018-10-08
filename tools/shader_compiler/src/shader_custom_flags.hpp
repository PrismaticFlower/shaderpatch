#pragma once

#include "compose_exception.hpp"

#include <algorithm>
#include <array>
#include <bitset>
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

#include <gsl/gsl>
#include <nlohmann/json.hpp>

namespace sp::shader {

class Custom_flags {
public:
   using Flag_type = std::uint32_t;

   constexpr static int max_flags = 16;

   Custom_flags() = default;

   template<typename Container>
   explicit Custom_flags(const Container& container)
   {
      static_assert(
         std::is_convertible_v<typename Container::value_type, std::string>,
         "Container::value_type must be convertible to std::string.");

      Expects(container.size() <= static_cast<std::size_t>(max_flags));

      _flag_count = static_cast<int>(container.size());
      std::copy(std::begin(container), std::end(container), _flag_names.begin());
   }

   int count() const noexcept
   {
      return _flag_count;
   }

   auto list_flags() const noexcept -> gsl::span<const std::string>
   {
      return gsl::span{_flag_names.data(), _flag_count};
   }

   auto generate_variations() const noexcept
      -> std::vector<std::pair<Preprocessor_defines, Flag_type>>
   {
      Expects(count() != 0);

      using namespace std::literals;

      std::vector<std::pair<Preprocessor_defines, Flag_type>> variations;

      const auto variation_count = static_cast<Flag_type>(1 << _flag_count);

      for (Flag_type i = 0; i < variation_count; ++i) {
         std::bitset<max_flags> flags{i};

         Preprocessor_defines defines;

         for (auto flag = 0; flag < _flag_count; ++flag) {
            defines.add_define(_flag_names[flag], flags[flag] ? "1"s : "0"s);
         }

         variations.emplace_back(std::move(defines), i);
      }

      return variations;
   }

private:
   int _flag_count = 0;
   std::array<std::string, max_flags> _flag_names;
};

template<typename Variation_type>
inline auto get_custom_variations(const Custom_flags& flags) noexcept
   -> std::vector<Variation_type>
{
   static_assert(sizeof(Variation_type::static_flags) * CHAR_BIT >= Custom_flags::max_flags,
                 "Variation_type::static_flags does not have enough bits to "
                 "represent Custom_flags.");

   auto variations = flags.generate_variations();

   std::vector<Variation_type> wrapped_variations;
   wrapped_variations.reserve(variations.size());

   for (auto& vari : variations) {
      auto& wrapped_vari = wrapped_variations.emplace_back();

      wrapped_vari.defines = std::move(vari.first);
      wrapped_vari.static_flags =
         static_cast<decltype(Variation_type::static_flags)>(vari.second);
   }

   return wrapped_variations;
}

inline void from_json(const nlohmann::json& j, Custom_flags& custom_flags)
{
   using namespace std::literals;

   if (!j.is_array()) {
      throw std::runtime_error{"flags list is not an array!"s};
   }
   else if (j.size() > static_cast<std::size_t>(Custom_flags::max_flags)) {
      throw compose_exception<std::runtime_error>("too many flags max is "s,
                                                  Custom_flags::max_flags);
   }

   custom_flags = Custom_flags{j.get<std::vector<std::string>>()};
}

}
