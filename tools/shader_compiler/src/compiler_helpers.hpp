#pragma once

#include "com_ptr.hpp"

#include <algorithm>
#include <ctime>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/functional/hash.hpp>
#include <d3dcompiler.h>
#include <gsl/gsl>
#include <nlohmann/json.hpp>

inline bool operator==(const D3D_SHADER_MACRO& l, const D3D_SHADER_MACRO& r)
{
   if ((!l.Name || !r.Name) || (!l.Definition || !r.Definition)) {
      return std::tie(l.Name, l.Definition) == std::tie(r.Name, r.Definition);
   }

   using Sv = std::string_view;

   return std::make_tuple(Sv{l.Name}, Sv{l.Definition}) ==
          std::make_tuple(Sv{r.Name}, Sv{r.Definition});
}

namespace sp {

const auto compiler_flags = D3DCOMPILE_OPTIMIZATION_LEVEL3;

struct Shader_macro {
   std::string name;
   std::string definition;

   operator D3D_SHADER_MACRO() const noexcept
   {
      return {name.data(), definition.data()};
   };
};

inline void from_json(const nlohmann::json& j, Shader_macro& macro)
{
   if (j.is_string()) {
      macro.name = j;
   }
   else {
      macro.name = j[0];
      macro.definition = j[1];
   }
}

struct Shader_cache_index {
   std::string entry_point;
   std::vector<D3D_SHADER_MACRO> definitions;
};

inline bool operator==(const Shader_cache_index& l, const Shader_cache_index& r)
{
   return std::tie(l.entry_point, l.definitions) ==
          std::tie(r.entry_point, r.definitions);
}

inline gsl::span<DWORD> make_dword_span(ID3DBlob& blob)
{
   using namespace std::literals;

   if (blob.GetBufferSize() % 4) {
      throw std::runtime_error{"Resulting shader bytecode was bad."s};
   }

   return gsl::make_span<DWORD>(static_cast<DWORD*>(blob.GetBufferPointer()),
                                static_cast<std::ptrdiff_t>(blob.GetBufferSize()) / 4);
}

inline auto read_definition_file(const boost::filesystem::path& path) -> nlohmann::json
{
   namespace fs = boost::filesystem;

   Expects(fs::exists(path));

   std::ifstream file{path.string()};

   nlohmann::json config;
   file >> config;

   return config;
}

inline auto date_test_shader_file(const boost::filesystem::path& file_path) noexcept
   -> std::time_t
{
   using namespace std::literals;

   namespace fs = boost::filesystem;

   Expects(fs::is_regular_file(file_path));

   class Includer : public ID3DInclude {
   public:
      Includer(fs::path relative_path)
         : _relative_path{std::move(relative_path)} {};

      HRESULT __stdcall Open(D3D_INCLUDE_TYPE, LPCSTR zfile_name, LPCVOID,
                             LPCVOID* out_data, UINT* out_size) override
      {
         std::string file_name{zfile_name};

         if (_opened_files.count(file_name)) {
            *out_data = _opened_files[file_name].data();
            *out_size = _opened_files[file_name].size();

            return S_OK;
         }

         const auto file_path = _relative_path / file_name;

         if (!fs::exists(file_path) || !fs::is_regular_file(file_path)) {
            *out_data = "";
            *out_size = sizeof("");

            return S_OK;
         }

         _newest_time = std::max(_newest_time, fs::last_write_time(file_path));

         fs::load_string_file(file_path, _opened_files[file_name]);

         *out_data = _opened_files[file_name].data();
         *out_size = _opened_files[file_name].size();

         return S_OK;
      }

      HRESULT __stdcall Close(LPCVOID) noexcept override
      {
         return S_OK;
      }

      std::time_t newest() const noexcept
      {
         return _newest_time;
      }

   private:
      fs::path _relative_path{};
      std::time_t _newest_time{0};
      std::unordered_map<std::string, std::string> _opened_files;
   };

   Includer includer{file_path.parent_path()};

   std::string file_contents;

   fs::load_string_file(file_path, file_contents);

   Com_ptr<ID3DBlob> blob;

   D3DPreprocess(file_contents.data(), file_contents.size(), nullptr, nullptr,
                 &includer, blob.clear_and_assign(), nullptr);

   return std::max(includer.newest(), fs::last_write_time(file_path));
}
}

template<>
struct std::hash<sp::Shader_cache_index> {
   using argument_type = sp::Shader_cache_index;
   using result_type = std::size_t;

   result_type operator()(argument_type const& entry) const noexcept
   {
      std::size_t seed = 0;

      boost::hash_combine(seed, std::hash<std::string>{}(entry.entry_point));

      for (const auto& item : entry.definitions) {
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
