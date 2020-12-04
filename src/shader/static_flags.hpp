#pragma once

#include "../logger.hpp"

#include <algorithm>
#include <array>
#include <bitset>
#include <cstdint>
#include <limits>
#include <string>

namespace sp::shader {

enum class Static_flag_op : std::uint8_t { set, clear };

class Static_flags {
public:
   using flags_type = std::uint64_t;

   constexpr static auto max_flags = std::numeric_limits<flags_type>::digits;

   template<typename T>
   explicit Static_flags(const T& t) noexcept
   {
      if (t.size() > max_flags) {
         log_and_terminate("Too many shader static flags!"sv);
      }

      _flag_count = t.size();

      std::ranges::copy(t, _flags.begin());
   }

   template<typename T>
   void get_flags_defines(const flags_type flag_values, T& output) const noexcept
   {
      std::bitset<max_flags> bits{flag_values};

      for (std::size_t i = 0; i < _flag_count; ++i) {
         output.emplace_back(_flags[i].data(), bits[i] ? "1" : "0");
      }
   }

private:
   std::size_t _flag_count = 0;
   std::array<std::string, max_flags> _flags;
};

}
