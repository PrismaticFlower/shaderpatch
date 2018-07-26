
#include "patch_compiler.hpp"
#include "algorithm.hpp"
#include "com_ptr.hpp"
#include "compiler_helpers.hpp"
#include "compose_exception.hpp"
#include "synced_io.hpp"
#include "ucfb_writer.hpp"
#include "volume_resource.hpp"

#include "preprocessor_defines.hpp"

#include <algorithm>
#include <execution>
#include <filesystem>
#include <functional>
#include <future>
#include <iostream>
#include <sstream>
#include <unordered_map>

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/range/algorithm.hpp>

namespace sp {

using namespace std::literals;
namespace fs = std::filesystem;

namespace {

auto compile_shader_impl(const std::string& entry_point,
                         std::vector<D3D_SHADER_MACRO> defines,
                         const fs::path& source_path, std::mutex& shaders_mutex,
                         std::vector<std::pair<std::size_t, std::vector<DWORD>>>& shaders,
                         std::unordered_map<Shader_cache_index, std::size_t>& cache,
                         std::string target) -> std::size_t
{
   std::unique_lock<std::mutex> lock{shaders_mutex};

   Shader_cache_index cache_index{entry_point, std::move(defines)};

   const auto cached = cache.find(cache_index);

   if (cached != std::end(cache)) return cached->second;

   lock.unlock();

   Com_ptr<ID3DBlob> error_message;
   Com_ptr<ID3DBlob> shader;

   auto result =
      D3DCompileFromFile(source_path.c_str(), cache_index.definitions.data(),
                         D3D_COMPILE_STANDARD_FILE_INCLUDE, entry_point.data(),
                         target.data(), compiler_flags, 0, shader.clear_and_assign(),
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

   const auto shader_index = shaders.size();

   cache.emplace(std::move(cache_index), shader_index);
   shaders.emplace_back(shader_index,
                        std::vector<DWORD>{std::cbegin(span), std::cend(span)});

   return shader_index;
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

   Preprocessor_defines shader_defines;

   shader_defines.add_defines(
      definition.value<std::vector<Shader_define>>("defines"s, {}));
   shader_defines.add_undefines(
      definition.value<std::vector<std::string>>("undefines"s, {}));

   for_each_exception_capable(std::execution::par, definition["states"s],
                              [&](const auto& state_def) {
                                 auto state = compile_state(state_def, shader_defines);

                                 std::lock_guard<std::mutex> lock{_states_mutex};

                                 _states.emplace_back(std::move(state));
                              });

   optimize_and_linearize_permutations();
   save(output_path);
}

void Patch_compiler::optimize_and_linearize_permutations()
{
   std::scoped_lock<std::mutex, std::mutex, std::mutex> lock{_vs_mutex, _ps_mutex,
                                                             _states_mutex};

   _vs_cache.clear();
   _ps_cache.clear();

   const auto optimize = [](std::vector<std::pair<std::size_t, std::vector<DWORD>>>& shaders,
                            auto remapper) {
      for (auto a_it = std::begin(shaders); a_it != std::end(shaders); ++a_it) {
         for (auto b_it = a_it + 1; b_it != std::end(shaders);) {
            if (std::equal(std::begin(a_it->second), std::end(a_it->second),
                           std::begin(b_it->second), std::end(b_it->second))) {
               remapper(b_it->first, a_it->first);

               b_it = shaders.erase(b_it);
            }
            else {
               ++b_it;
            }
         }
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

         vs_writer.emplace_child("SIZE"_mn).write(
            static_cast<std::uint32_t>(_vs_shaders.size()));

         for (const auto& bytecode : _vs_shaders) {
            auto bc_writer = vs_writer.emplace_child("BC__"_mn);

            bc_writer.emplace_child("ID__"_mn).write(
               static_cast<std::uint32_t>(bytecode.first));
            bc_writer.emplace_child("CODE"_mn).write(gsl::make_span(bytecode.second));
         }
      }

      // write pixel shaders
      {
         auto ps_writer = writer.emplace_child("PSHD"_mn);

         ps_writer.emplace_child("SIZE"_mn).write(
            static_cast<std::uint32_t>(_ps_shaders.size()));

         for (const auto& bytecode : _ps_shaders) {
            auto bc_writer = ps_writer.emplace_child("BC__"_mn);

            bc_writer.emplace_child("ID__"_mn).write(
               static_cast<std::uint32_t>(bytecode.first));
            bc_writer.emplace_child("CODE"_mn).write(gsl::make_span(bytecode.second));
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
                                   const Preprocessor_defines& global_defines)
   -> Patch_compiler::State
{
   const std::string vs_entry_point = state_def["vertex_shader"s];
   const std::string ps_entry_point = state_def["pixel_shader"s];

   Preprocessor_defines state_defines;

   state_defines.combine_with(global_defines);
   state_defines.add_defines(
      state_def.value<std::vector<Shader_define>>("defines"s, {}));
   state_defines.add_undefines(
      state_def.value<std::vector<std::string>>("undefines"s, {}));

   State state;

   state.name = state_def["name"s];

   state.shaders.reserve(_variations.size());

   const auto predicate = [&](const auto& variation) {
      Shader shader;

      shader.flags = variation.flags;

      shader.vs_index =
         compile_vertex_shader(vs_entry_point,
                               combine_defines(variation.defines, state_defines).get());
      shader.ps_index = compile_pixel_shader(
         ps_entry_point, combine_defines(variation.ps_defines, state_defines).get());

      state.shaders.emplace_back(std::move(shader));
   };

   for_each_exception_capable(std::execution::par, _variations, predicate);

   return state;
}

auto Patch_compiler::compile_vertex_shader(const std::string& entry_point,
                                           const std::vector<D3D_SHADER_MACRO>& defines)
   -> std::size_t
{
   return compile_shader_impl(entry_point, std::move(defines), _source_path,
                              _vs_mutex, _vs_shaders, _vs_cache, "vs_3_0"s);
}

auto Patch_compiler::compile_pixel_shader(const std::string& entry_point,
                                          const std::vector<D3D_SHADER_MACRO>& defines)
   -> std::size_t
{
   return compile_shader_impl(entry_point, std::move(defines), _source_path,
                              _ps_mutex, _ps_shaders, _ps_cache, "ps_3_0"s);
}
}
