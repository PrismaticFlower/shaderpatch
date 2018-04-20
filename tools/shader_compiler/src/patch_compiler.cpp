
#include "patch_compiler.hpp"
#include "algorithm.hpp"
#include "com_ptr.hpp"
#include "compiler_helpers.hpp"
#include "compose_exception.hpp"
#include "synced_io.hpp"
#include "ucfb_writer.hpp"
#include "volume_resource.hpp"

#include <algorithm>
#include <execution>
#include <functional>
#include <future>
#include <iostream>
#include <sstream>
#include <unordered_map>

#include <boost/container/flat_map.hpp>

namespace sp {

using namespace std::literals;
namespace fs = boost::filesystem;

namespace {

auto compile_shader_impl(const std::string& entry_point,
                         const std::vector<D3D_SHADER_MACRO>& defines,
                         const boost::filesystem::path& source_path,
                         std::mutex& shaders_cache_mutex,
                         std::vector<std::vector<DWORD>>& shaders,
                         std::unordered_map<std::size_t, std::size_t>& cache,
                         std::string target) -> std::size_t
{
   const auto key = compiler_cache_hash(entry_point, defines);

   std::unique_lock<std::mutex> lock{shaders_cache_mutex};

   const auto cached = cache.find(key);

   if (cached != std::end(cache)) return cached->second;

   lock.unlock();

   Com_ptr<ID3DBlob> error_message;
   Com_ptr<ID3DBlob> shader;

   auto result = D3DCompileFromFile(source_path.c_str(), defines.data(),
                                    D3D_COMPILE_STANDARD_FILE_INCLUDE,
                                    entry_point.data(), target.data(),
                                    compiler_flags, 0, shader.clear_and_assign(),
                                    error_message.clear_and_assign());

   if (result != S_OK) {
      if (!error_message) {
         throw std::runtime_error{"Unknown compiler error occured."s};
      }

      throw std::runtime_error{static_cast<char*>(error_message->GetBufferPointer())};
   }
   else if (error_message) {
      std::cout << static_cast<char*>(error_message->GetBufferPointer());
   }

   const auto span = make_dword_span(*shader);

   lock.lock();

   const auto shaded_index = cache[key] = shaders.size();

   shaders.emplace_back(std::cbegin(span), std::cend(span));

   return shaded_index;
}
}

Patch_compiler::Patch_compiler(nlohmann::json definition, const fs::path& definition_path,
                               const fs::path& source_file_dir,
                               const fs::path& output_dir)
{
   Expects(fs::is_directory(source_file_dir) && fs::is_directory(output_dir));

   _source_path =
      source_file_dir /
      definition.value("source_name"s,
                       definition_path.stem().replace_extension(".fx"s).string());

   if (!fs::exists(_source_path)) {
      throw compose_exception<std::runtime_error>("Source file "sv, _source_path,
                                                  " does not exist"sv);
   }

   const auto output_path =
      output_dir / definition_path.stem().replace_extension(".shader"s);

   if (fs::exists(output_path) && (std::max(fs::last_write_time(definition_path),
                                            date_test_shader_file(_source_path)) <
                                   fs::last_write_time(output_path))) {
      return;
   }

   synced_print("Munging shader "sv, definition_path.filename().string(), "..."sv);

   _render_type = definition["rendertype"s];

   _variations = get_shader_variations(definition["skinned"s], definition["lighting"s],
                                       definition["vertex_color"s]);

   std::vector<std::string> shader_defines =
      definition.value<std::vector<std::string>>("defines"s, {});

   for_each_exception_capable(std::execution::par, definition["states"s],
                              [&](const auto& state_def) {
                                 auto state = compile_state(state_def, shader_defines);

                                 std::lock_guard<std::mutex> lock{_states_mutex};

                                 _states.emplace_back(std::move(state));
                              });

   // optimize_permutations();
   save(output_path);
}

void Patch_compiler::optimize_permutations()
{
   _vs_cache.clear();
   _ps_cache.clear();

   const auto optimize = [](std::vector<std::vector<DWORD>>& shaders, auto remapper) {
      boost::container::flat_map<std::size_t, std::vector<DWORD>> stable_shaders;
      stable_shaders.reserve(shaders.size());

      for (auto i = 0u; i < shaders.size(); ++i) {
         stable_shaders[i] = std::move(shaders[i]);
      }

      const auto remove_next_duplicate = [&] {
         for (auto& a : stable_shaders) {
            for (auto& b : stable_shaders) {
               if (a.first == b.first) continue;

               if (std::equal(std::cbegin(a.second), std::cend(a.second),
                              std::cbegin(b.second), std::cend(b.second))) {
                  remapper(b.first, a.first);

                  stable_shaders.erase(b.first);

                  return true;
               }
            }
         }

         return false;
      };

      while (remove_next_duplicate())
         ;

      shaders.clear();

      auto iter = std::begin(stable_shaders);

      for (auto i = 0u; i < stable_shaders.size(); ++i, ++iter) {
         remapper(iter->first, i);

         std::swap(shaders.emplace_back(), iter->second);
      }
   };

   optimize(_vs_shaders, [this](std::size_t from, std::size_t to) {
      for (auto& state : _states) {
         for (auto& shader : state.shaders) {
            if (shader.vs_index == from) shader.vs_index = to;
         }
      }
   });

   optimize(_ps_shaders, [this](std::size_t from, std::size_t to) {
      for (auto& state : _states) {
         for (auto& shader : state.shaders) {
            if (shader.ps_index == from) shader.ps_index = to;
         }
      }
   });

   return;
}

void Patch_compiler::save(const fs::path& output_path) const
{
   std::ostringstream shader_chunk;

   // create shader chunk
   {
      ucfb::Writer writer{shader_chunk, "ssdr"_mn};

      writer.emplace_child("RTYP"_mn).write(_render_type);

      // write vertex shaders
      {
         auto vs_writer = writer.emplace_child("VSHD"_mn);

         vs_writer.emplace_child("INFO"_mn).write(
            static_cast<std::uint32_t>(_vs_shaders.size()));

         for (const auto& bytecode : _vs_shaders) {
            vs_writer.emplace_child("VSBC"_mn).write(gsl::make_span(bytecode));
         }
      }

      // write pixel shaders
      {
         auto ps_writer = writer.emplace_child("PSHD"_mn);

         ps_writer.emplace_child("INFO"_mn).write(
            static_cast<std::uint32_t>(_ps_shaders.size()));

         for (const auto& bytecode : _ps_shaders) {
            ps_writer.emplace_child("PSBC"_mn).write(gsl::make_span(bytecode));
         }
      }

      // write shader references
      {
         auto states_writer = writer.emplace_child("SHRS"_mn);

         for (const auto& state : _states) {
            auto shader_writer = states_writer.emplace_child("SHDR"_mn);

            shader_writer.emplace_child("NAME"_mn).write(state.name);
            shader_writer.emplace_child("INFO"_mn).write(
               static_cast<std::uint32_t>(state.shaders.size()));

            for (const auto& shader : state.shaders) {
               shader_writer.emplace_child("VARI"_mn)
                  .write(shader.flags, static_cast<std::uint32_t>(shader.vs_index),
                         static_cast<std::uint32_t>(shader.ps_index));
            }
         }
      }
   }

   const auto data = shader_chunk.str();

   save_volume_resource(output_path.string(), _render_type, Volume_resource_type::shader,
                        gsl::make_span(reinterpret_cast<const std::byte*>(data.data()),
                                       data.size()));
}

auto Patch_compiler::compile_state(const nlohmann::json& state_def,
                                   const std::vector<std::string>& global_defines)
   -> Patch_compiler::State
{
   const std::string vs_entry_point = state_def["vertex_shader"s];
   const std::string ps_entry_point = state_def["pixel_shader"s];

   std::vector<std::string> state_defines =
      state_def.value<std::vector<std::string>>("defines"s, {});

   State state;

   state.name = state_def["name"s];

   state.shaders.reserve(_variations.size());

   const auto predicate = [&](const auto& variation) {
      Shader shader;

      shader.flags = variation.flags;

      std::vector<D3D_SHADER_MACRO> defines;
      defines.reserve(variation.definitions.size() + state_defines.size() +
                      global_defines.size());
      defines = variation.definitions;

      defines.pop_back();

      for (auto& def : global_defines) {
         defines.emplace_back() = {def.c_str(), nullptr};
      }

      for (auto& def : state_defines) {
         defines.emplace_back() = {def.c_str(), nullptr};
      }

      defines.emplace_back() = {nullptr, nullptr};

      shader.vs_index = compile_vertex_shader(vs_entry_point, defines);
      shader.ps_index = compile_pixel_shader(ps_entry_point, defines);

      state.shaders.emplace_back(std::move(shader));
   };

   for_each_exception_capable(std::execution::par, _variations, predicate);

   return state;
}

auto Patch_compiler::compile_vertex_shader(const std::string& entry_point,
                                           const std::vector<D3D_SHADER_MACRO>& defines)
   -> std::size_t
{
   return compile_shader_impl(entry_point, defines, _source_path, _vs_mutex,
                              _vs_shaders, _vs_cache, "vs_3_0"s);
}

auto Patch_compiler::compile_pixel_shader(const std::string& entry_point,
                                          const std::vector<D3D_SHADER_MACRO>& defines)
   -> std::size_t
{
   return compile_shader_impl(entry_point, defines, _source_path, _ps_mutex,
                              _ps_shaders, _ps_cache, "ps_3_0"s);
}
}
