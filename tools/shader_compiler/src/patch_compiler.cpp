
#include "patch_compiler.hpp"
#include "com_ptr.hpp"
#include "compiler_helpers.hpp"
#include "ucfb_writer.hpp"
#include "volume_resource.hpp"

#include <algorithm>
#include <functional>
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
                         std::vector<std::vector<DWORD>>& shaders,
                         std::unordered_map<std::size_t, std::size_t>& cache,
                         std::string target) -> std::size_t
{

   const auto key = compiler_cache_hash(entry_point, defines);

   const auto cached = cache.find(key);

   if (cached != std::end(cache)) return cached->second;

   cache[key] = shaders.size();

   Com_ptr<ID3DBlob> error_message;
   Com_ptr<ID3DBlob> shader;

   auto result = D3DCompileFromFile(source_path.c_str(), defines.data(),
                                    D3D_COMPILE_STANDARD_FILE_INCLUDE,
                                    entry_point.data(), target.data(),
                                    compiler_flags, 0, shader.clear_and_assign(),
                                    error_message.clear_and_assign());

   if (result != S_OK) {
      throw std::runtime_error{static_cast<char*>(error_message->GetBufferPointer())};
   }
   else if (error_message) {
      std::cout << static_cast<char*>(error_message->GetBufferPointer());
   }

   const auto span = make_dword_span(*shader);

   shaders.emplace_back(std::cbegin(span), std::cend(span));

   return cache[key];
}
}

Patch_compiler::Patch_compiler(std::string definition_path)
{
   const auto definition = read_definition_file(definition_path);

   _source_path = (std::string{} = definition["source_path"s]);
   _render_type = definition["rendertype"s];

   _variations = get_shader_variations(definition["skinned"s], definition["lighting"s],
                                       definition["vertex_color"s]);

   for (const auto& state_def : definition["states"s]) {
      _states.emplace_back(compile_state(state_def));
   }
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

void Patch_compiler::save(std::string_view output_path) const
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

   save_volume_resource(std::string{output_path}, _render_type,
                        Volume_resource_type::shader,
                        gsl::make_span(reinterpret_cast<const std::byte*>(data.data()),
                                       data.size()));
}

auto Patch_compiler::compile_state(const nlohmann::json& state_def)
   -> Patch_compiler::State
{
   const std::string vs_entry_point = state_def["vertex_shader"s];
   const std::string ps_entry_point = state_def["pixel_shader"s];

   std::vector<std::string> state_defines =
      state_def.value<std::vector<std::string>>("defines"s, {});

   State state;

   state.name = state_def["name"s];
   state.shaders.reserve(_variations.size());

   for (const auto& variation : _variations) {
      Shader shader;

      shader.flags = variation.flags;

      std::vector<D3D_SHADER_MACRO> defines;
      defines.reserve(variation.definitions.size() + state_defines.size());
      defines = variation.definitions;

      defines.pop_back();

      for (auto& def : state_defines) {
         defines.emplace_back() = {def.c_str(), nullptr};
      }

      defines.emplace_back() = {nullptr, nullptr};

      shader.vs_index = compile_vertex_shader(vs_entry_point, defines);
      shader.ps_index = compile_pixel_shader(ps_entry_point, defines);

      state.shaders.emplace_back(std::move(shader));
   }

   return state;
}

auto Patch_compiler::compile_vertex_shader(const std::string& entry_point,
                                           const std::vector<D3D_SHADER_MACRO>& defines)
   -> std::size_t
{
   return compile_shader_impl(entry_point, defines, _source_path, _vs_shaders,
                              _vs_cache, "vs_3_0"s);
}

auto Patch_compiler::compile_pixel_shader(const std::string& entry_point,
                                          const std::vector<D3D_SHADER_MACRO>& defines)
   -> std::size_t
{
   return compile_shader_impl(entry_point, defines, _source_path, _ps_shaders,
                              _ps_cache, "ps_3_0"s);
}
}
