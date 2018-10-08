#pragma once

#include "compiler_helpers.hpp"

#include <algorithm>
#include <array>
#include <iterator>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

#include <d3dcompiler.h>

#include <nlohmann/json.hpp>

namespace sp {

struct Shader_define {
   Shader_define() = default;

   Shader_define(std::string name, std::string definition) noexcept
      : name{std::move(name)}, definition{std::move(definition)} {};

   template<typename First, typename Second>
   Shader_define(const std::pair<First, Second>& pair) noexcept
      : name{pair.first}, definition{pair.second}
   {
   }

   Shader_define(const std::array<std::string, 2>& pair) noexcept
      : name{pair[0]}, definition{pair[1]}
   {
   }

   std::string name;
   std::string definition;
};

class Preprocessor_defines {
public:
   template<typename Defines>
   void add_defines(const Defines& defines) noexcept
   {
      for (const Shader_define& def : defines) {
         _defines[def.name] = def.definition;
      }
   }

   void add_define(const std::string& name, std::string definition) noexcept
   {
      _defines[name] = std::move(definition);
   }

   template<typename Defines>
   void move_in_defines(Defines&& defines) noexcept
   {
      for (Shader_define& def : defines) {
         _defines[std::move(def.name)] = std::move(def.definition);
      }
   }

   template<typename Undefines>
   void add_undefines(const Undefines& undefines) noexcept
   {
      _undefines.insert(std::cbegin(undefines), std::cend(undefines));
   }

   template<typename Undefines>
   void move_in_undefines(Undefines&& undefines) noexcept
   {
      for (std::string& undef : undefines) {
         _undefines.emplace(std::move(undef));
      }
   }

   void combine_with(const Preprocessor_defines& other) noexcept
   {
      add_defines(other._defines);
      add_undefines(other._undefines);
   }

   auto get() const noexcept -> std::vector<D3D_SHADER_MACRO>
   {
      std::vector<D3D_SHADER_MACRO> defines;
      defines.reserve(_defines.size());

      for (const auto& def : _defines) {
         if (_undefines.count(def.first)) continue;

         defines.emplace_back(D3D_SHADER_MACRO{def.first.c_str(), def.second.c_str()});
      }

      defines.emplace_back(D3D_SHADER_MACRO{nullptr, nullptr});

      return defines;
   }

private:
   std::unordered_map<std::string, std::string> _defines;
   std::unordered_set<std::string> _undefines;
};

inline auto combine_defines(const Preprocessor_defines& left,
                            const Preprocessor_defines& right) noexcept -> Preprocessor_defines
{
   Preprocessor_defines defines;

   defines.combine_with(left);
   defines.combine_with(right);

   return defines;
}

inline void from_json(const nlohmann::json& j, Shader_define& define)
{
   if (j.is_string()) {
      define.name = j.get<std::string>();
   }
   else {
      define.name = j[0].get<std::string>();
      define.definition = j[1].get<std::string>();
   }
}
}
