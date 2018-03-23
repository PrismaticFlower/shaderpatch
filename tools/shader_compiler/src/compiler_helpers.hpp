#pragma once

#include <fstream>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/functional/hash.hpp>
#include <d3dcompiler.h>
#include <gsl/gsl>
#include <nlohmann/json.hpp>

template<>
struct std::hash<std::vector<D3D_SHADER_MACRO>> {
   using argument_type = std::vector<D3D_SHADER_MACRO>;
   using result_type = std::size_t;

   result_type operator()(argument_type const& vec) const noexcept
   {
      std::size_t seed = 0;

      for (const auto& item : vec) {
         if (item.Name) {
            boost::hash_combine(seed, std::hash<std::string_view>{}(item.Name));
         }
         if (item.Definition) {
            boost::hash_combine(seed, std::hash<std::string_view>{}(item.Definition));
         }
      }

      return seed;
   }
};

namespace sp {

const auto compiler_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_OPTIMIZATION_LEVEL3;

using namespace std::literals;

namespace fs = boost::filesystem;

template<typename... Args>
inline std::size_t compiler_cache_hash(Args&&... args)
{
   const auto hash = [](std::size_t& seed, auto value) {
      boost::hash_combine(seed,
                          std::hash<std::remove_reference_t<decltype(value)>>{}(value));
   };

   std::size_t seed = 0;

   (hash(seed, std::forward<Args>(args)), ...);

   return seed;
}

inline gsl::span<DWORD> make_dword_span(ID3DBlob& blob)
{
   if (blob.GetBufferSize() % 4) {
      throw std::runtime_error{"Resulting shader bytecode was bad."s};
   }

   return gsl::make_span<DWORD>(static_cast<DWORD*>(blob.GetBufferPointer()),
                                static_cast<std::ptrdiff_t>(blob.GetBufferSize()) / 4);
}

inline auto read_source_file(std::string_view file_name) -> std::string
{
   const auto path = fs::path{std::cbegin(file_name), std::cend(file_name)};

   if (!fs::exists(path)) {
      throw std::runtime_error{"Source file does not exist."s};
   }

   return {std::istreambuf_iterator<char>(std::ifstream{path.string(), std::ios::binary}),
           std::istreambuf_iterator<char>()};
}

inline auto read_definition_file(std::string_view file_name) -> nlohmann::json
{
   const auto path = fs::path{std::cbegin(file_name), std::cend(file_name)};

   if (!fs::exists(path)) {
      throw std::runtime_error{"Source file does not exist."s};
   }

   std::ifstream file{path.string()};

   nlohmann::json config;
   file >> config;

   return config;
}
}
