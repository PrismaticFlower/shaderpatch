#pragma once

#include "compose_exception.hpp"

#include <algorithm>
#include <array>
#include <bitset>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <gsl/gsl>
#include <nlohmann/json.hpp>

namespace sp::shader {

enum class Flag_op { set, clear };

class Custom_flags {
public:
   using Flag_type = std::uint32_t;

   constexpr static int max_flags = 16;

   Custom_flags() = default;

   Custom_flags(std::vector<std::string> flags,
                std::unordered_map<std::string, std::unordered_map<std::string, Flag_op>> flag_ops)
   {
      using namespace std::literals;

      if (flags.size() > static_cast<std::size_t>(Custom_flags::max_flags)) {
         throw compose_exception<std::runtime_error>("too many flags max is "s,
                                                     Custom_flags::max_flags);
      }

      _flag_count = static_cast<int>(flags.size());

      for (auto i = 0u; i < _flag_count; ++i)
         std::swap(_flag_names[i], flags[i]);

      for (auto& flag_op : flag_ops) {
         auto& flag_refs = _flag_ops[flag_index(flag_op.first)];
         flag_refs.reserve(flag_op.second.size());

         for (auto& flag_ref : flag_op.second)
            flag_refs.emplace_back(flag_index(flag_ref.first), flag_ref.second);
      }
   }

   auto count() const noexcept -> std::size_t
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
      std::unordered_set<std::bitset<max_flags>> added_variations;

      const auto variation_count = static_cast<Flag_type>(1 << _flag_count);

      for (Flag_type i = 0; i < variation_count; ++i) {
         const std::bitset<max_flags> flags =
            apply_flag_ops(std::bitset<max_flags>{i});

         if (const auto [it, inserted] = added_variations.emplace(flags); !inserted)
            continue;

         Preprocessor_defines defines;

         for (auto flag = 0u; flag < _flag_count; ++flag) {
            defines.add_define(_flag_names[flag], flags[flag] ? "1"s : "0"s);
         }

         variations.emplace_back(std::move(defines), flags.to_ulong());
      }

      return variations;
   }

private:
   auto flag_index(const std::string_view flag) const -> std::size_t
   {
      using namespace std::literals;

      for (std::size_t i = 0; i < _flag_names.size(); ++i)
         if (_flag_names[i] == flag) return i;

      throw compose_exception<std::runtime_error>(std::quoted(flag),
                                                  " is not a valid flag"s);
   }

   auto apply_flag_ops(std::bitset<max_flags> flags) const noexcept
      -> std::bitset<max_flags>
   {
      for (auto flag = 0u; flag < _flag_count; ++flag) {
         if (!flags[flag]) continue;

         auto& ops = _flag_ops[flag];

         for (const auto& op : ops) {
            if (op.second == Flag_op::set) {
               std::bitset<max_flags> mask{};
               mask[op.first] = true;

               flags |= mask;
            }
            else if (op.second == Flag_op::clear) {
               std::bitset<max_flags> mask{};
               mask[op.first] = true;
               mask = ~mask;

               flags &= mask;
            }
         }
      }

      return flags;
   }

   std::size_t _flag_count = 0;
   std::array<std::string, max_flags> _flag_names;
   std::array<std::vector<std::pair<std::size_t, Flag_op>>, max_flags> _flag_ops;
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

inline void from_json(const nlohmann::json& j, Flag_op& flag_op)
{
   using namespace std::literals;

   const auto str = j.get<std::string>();

   if (str == "set"s)
      flag_op = Flag_op::set;
   else if (str == "clear"s)
      flag_op = Flag_op::clear;
   else
      throw std::runtime_error{"flag op is invalid!"s};
}

}
