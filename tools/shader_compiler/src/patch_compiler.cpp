
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

namespace sp {

using namespace std::literals;
namespace fs = std::filesystem;

namespace {

auto compile_shader_impl(DWORD compiler_flags, const std::string& entry_point,
                         std::vector<D3D_SHADER_MACRO> defines,
                         const fs::path& source_path, std::string target)
   -> Com_ptr<ID3DBlob>
{
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

   return shader;
}

auto resolve_state_flags(const std::unordered_map<std::string, bool>& state_flags,
                         const std::vector<std::string>& entrypoint_flag_names)
   -> std::uint32_t
{
   std::bitset<shader::Custom_flags::max_flags> result{};

   const auto count = static_cast<int>(entrypoint_flag_names.size());

   for (const auto& flag : state_flags) {
      for (auto i = 0; i < count; ++i) {
         if (entrypoint_flag_names[i] == flag.first) {
            result[i] = flag.second;
            break;
         }
      }
   }

   return result.to_ulong();
}

}

Patch_compiler::Patch_compiler(DWORD compiler_flags, nlohmann::json definition,
                               const fs::path& definition_path,
                               const fs::path& source_file_dir,
                               const fs::path& output_dir)
   : _compiler_flags{compiler_flags}
{
   Expects(fs::is_directory(source_file_dir) && fs::is_directory(output_dir));

   shader::Description description = definition;

   _group_name = description.group_name;
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

   compile_entrypoints(description.entrypoints, description.input_layouts);
   assemble_rendertypes(description.rendertypes);

   save(output_path);
}

void Patch_compiler::save(const fs::path& output_path) const
{
   std::ofstream output{output_path, std::ios::out | std::ios::binary};

   ucfb::Writer root_writer{output, "ucfb"_mn};

   // create shader chunk
   {
      auto writer = root_writer.emplace_child("shpk"_mn);

      writer.emplace_child("NAME"_mn).write(_group_name);

      // write entry points
      {
         auto entrypoint_writer = writer.emplace_child("EPTS"_mn);

         // write vertex entry points
         {
            auto vs_writer = entrypoint_writer.emplace_child("VSHD"_mn);

            write_entrypoints(vs_writer, _vertex_entrypoints);
         }

         // write pixel entry points
         {
            auto ps_writer = entrypoint_writer.emplace_child("PSHD"_mn);

            write_entrypoints(ps_writer, _pixel_entrypoints);
         }
      }

      // write rendertypes
      {
         auto rendertypes_writer = writer.emplace_child("RTS_"_mn);

         rendertypes_writer.emplace_child("SIZE"_mn).write(
            static_cast<std::uint32_t>(_rendertypes.size()));

         for (const auto& rendertype : _rendertypes) {
            auto rtyp_writer = rendertypes_writer.emplace_child("RTYP"_mn);

            rtyp_writer.emplace_child("NAME"_mn).write(rendertype.first);
            rtyp_writer.emplace_child("SIZE"_mn).write(
               static_cast<std::uint32_t>(rendertype.second.states.size()));

            // write rendertype states
            for (const auto& state : rendertype.second.states) {
               auto stat_writer = rtyp_writer.emplace_child("STAT"_mn);

               stat_writer.emplace_child("NAME"_mn).write(state.first);

               // write state info
               {
                  auto info_writer = stat_writer.emplace_child("INFO"_mn);

                  info_writer.write(state.second.vs_entrypoint);
                  info_writer.write(state.second.vs_static_flags);
                  info_writer.write(state.second.ps_entrypoint);
                  info_writer.write(state.second.ps_static_flags);
               }
            }
         }
      }
   }
}

void Patch_compiler::compile_entrypoints(
   const std::unordered_map<std::string, shader::Entrypoint>& entrypoints,
   const std::unordered_map<std::string, std::vector<shader::Input_element>>& input_layouts)
{
   for (const auto& entrypoint : entrypoints) {
      switch (entrypoint.second.stage) {
      case shader::Stage::vertex:
         compile_vertex_entrypoint(entrypoint, input_layouts);
         break;
      case shader::Stage::pixel:
         compile_pixel_entrypoint(entrypoint);
         break;
      }
   }
}

void Patch_compiler::compile_vertex_entrypoint(
   const std::pair<std::string, shader::Entrypoint>& entrypoint,
   const std::unordered_map<std::string, std::vector<shader::Input_element>>& input_layouts)
{
   Expects(entrypoint.second.stage == shader::Stage::vertex);
   Expects(!_vertex_entrypoints.count(entrypoint.first));

   auto& state = std::get<shader::Vertex_state>(entrypoint.second.stage_state);

   auto variations = shader::get_vertex_shader_variations(state);

   if (entrypoint.second.static_flags.count()) {
      variations =
         shader::combine_variations(variations,
                                    shader::get_custom_variations<shader::Vertex_variation>(
                                       entrypoint.second.static_flags));
   }

   Compiled_vertex_entrypoint compiled;

   const auto entrypoint_func =
      entrypoint.second.function_name.value_or(entrypoint.first);

   for (const auto& variation : variations) {
      auto input_layout = (state.input_layout != "$auto"sv)
                             ? input_layouts.at(state.input_layout)
                             : shader::vs_flags_to_input_layout(variation.flags);
      auto bytecode = compile_vertex_shader(entrypoint_func,
                                            combine_defines(entrypoint.second.defines,
                                                            variation.defines));

      compiled.variations[{variation.flags, variation.static_flags}] =
         Compiled_vertex_variation{std::move(input_layout), std::move(bytecode)};
   }

   const auto static_flags_names = entrypoint.second.static_flags.list_flags();
   compiled.static_flag_names.assign(std::cbegin(static_flags_names),
                                     std::cend(static_flags_names));

   _vertex_entrypoints.emplace(entrypoint.first, std::move(compiled));
}

void Patch_compiler::compile_pixel_entrypoint(
   const std::pair<std::string, shader::Entrypoint>& entrypoint)
{
   Expects(entrypoint.second.stage == shader::Stage::pixel);
   Expects(!_vertex_entrypoints.count(entrypoint.first));

   auto& state = std::get<shader::Pixel_state>(entrypoint.second.stage_state);

   auto variations = shader::get_pixel_shader_variations(state);

   if (entrypoint.second.static_flags.count()) {
      variations =
         shader::combine_variations(variations,
                                    shader::get_custom_variations<shader::Pixel_variation>(
                                       entrypoint.second.static_flags));
   }

   Compiled_pixel_entrypoint compiled;

   const auto entrypoint_func =
      entrypoint.second.function_name.value_or(entrypoint.first);

   for (const auto& variation : variations) {
      compiled.variations
         .emplace(std::pair{variation.flags, variation.static_flags},
                  compile_pixel_shader(entrypoint_func,
                                       combine_defines(entrypoint.second.defines,
                                                       variation.defines)));
   }

   const auto static_flags_names = entrypoint.second.static_flags.list_flags();
   compiled.static_flag_names.assign(std::cbegin(static_flags_names),
                                     std::cend(static_flags_names));

   _pixel_entrypoints.emplace(entrypoint.first, std::move(compiled));
}

auto Patch_compiler::compile_vertex_shader(const std::string& entry_point,
                                           Preprocessor_defines defines)
   -> Com_ptr<ID3DBlob>
{
   defines.add_define("__VERTEX_SHADER__"s, "1");

   return compile_shader_impl(_compiler_flags, entry_point, defines.get(),
                              _source_path, "vs_5_0"s);
}

auto Patch_compiler::compile_pixel_shader(const std::string& entry_point,
                                          Preprocessor_defines defines)
   -> Com_ptr<ID3DBlob>
{
   defines.add_define("__PIXEL_SHADER__"s, "1");

   return compile_shader_impl(_compiler_flags, entry_point, defines.get(),
                              _source_path, "ps_5_0"s);
}

void Patch_compiler::assemble_rendertypes(
   const std::unordered_map<std::string, shader::Rendertype>& rendertypes)
{
   for (const auto& rendertype : rendertypes) {
      Assembled_rendertype assembled_rt;

      for (const auto& state : rendertype.second.states) {
         Assembled_state assembled_state;

         assembled_state.vs_entrypoint = state.second.vs_entrypoint;
         assembled_state.vs_static_flags = resolve_state_flags(
            state.second.vs_static_flag_values,
            _vertex_entrypoints.at(assembled_state.vs_entrypoint).static_flag_names);

         assembled_state.ps_entrypoint = state.second.ps_entrypoint;
         assembled_state.ps_static_flags = resolve_state_flags(
            state.second.ps_static_flag_values,
            _pixel_entrypoints.at(assembled_state.ps_entrypoint).static_flag_names);

         assembled_rt.states.emplace(state.first, std::move(assembled_state));
      }

      _rendertypes.emplace(rendertype.first, std::move(assembled_rt));
   }
}

void Patch_compiler::write_entrypoints(
   ucfb::Writer& writer,
   const std::unordered_map<std::string, Compiled_vertex_entrypoint>& entrypoints) const
{
   writer.emplace_child("SIZE"_mn).write(
      static_cast<std::uint32_t>(entrypoints.size()));

   for (const auto& entrypoint : entrypoints) {
      auto ep_writer = writer.emplace_child("EP__"_mn);

      ep_writer.emplace_child("NAME"_mn).write(entrypoint.first);

      // write entry point variations
      {
         auto vrs_writer = ep_writer.emplace_child("VRS_"_mn);

         vrs_writer.emplace_child("SIZE"_mn).write(
            static_cast<std::uint32_t>(entrypoint.second.variations.size()));

         for (const auto& vari : entrypoint.second.variations) {
            auto vari_writer = vrs_writer.emplace_child("VARI"_mn);

            vari_writer.write(vari.first.first);
            vari_writer.write(vari.first.second);
            vari_writer.write(
               static_cast<std::uint32_t>(vari.second.bytecode->GetBufferSize()));
            vari_writer.write(gsl::span{static_cast<const std::byte*>(
                                           vari.second.bytecode->GetBufferPointer()),
                                        static_cast<gsl::index>(
                                           vari.second.bytecode->GetBufferSize())});

            // write variation input layout
            {
               auto ialo_writer = vari_writer.emplace_child("IALO"_mn);

               ialo_writer.write((vari.first.first & Vertex_shader_flags::compressed) ==
                                 Vertex_shader_flags::compressed);
               ialo_writer.write(
                  static_cast<std::uint32_t>(vari.second.input_layout.size()));

               for (const auto& elem : vari.second.input_layout) {
                  ialo_writer.write(elem.semantic.name);
                  ialo_writer.write(elem.semantic.index);
                  ialo_writer.write(elem.format);
                  ialo_writer.write(elem.offset);
               }
            }
         }
      }

      // write entry point flag names
      {
         auto flag_names_writer = ep_writer.emplace_child("FLGS"_mn);

         flag_names_writer.write(
            static_cast<std::uint32_t>(entrypoint.second.static_flag_names.size()));

         for (const auto& name : entrypoint.second.static_flag_names) {
            flag_names_writer.write(name);
         }
      }
   }
}

void Patch_compiler::write_entrypoints(
   ucfb::Writer& writer,
   const std::unordered_map<std::string, Compiled_pixel_entrypoint>& entrypoints) const
{
   writer.emplace_child("SIZE"_mn).write(
      static_cast<std::uint32_t>(entrypoints.size()));

   for (const auto& entrypoint : entrypoints) {
      auto ep_writer = writer.emplace_child("EP__"_mn);

      ep_writer.emplace_child("NAME"_mn).write(entrypoint.first);

      // write entry point variations
      {
         auto vrs_writer = ep_writer.emplace_child("VRS_"_mn);

         vrs_writer.emplace_child("SIZE"_mn).write(
            static_cast<std::uint32_t>(entrypoint.second.variations.size()));

         for (const auto& vari : entrypoint.second.variations) {
            auto vari_writer = vrs_writer.emplace_child("VARI"_mn);

            vari_writer.write(vari.first.first);
            vari_writer.write(vari.first.second);
            vari_writer.write(static_cast<std::uint32_t>(vari.second->GetBufferSize()));
            vari_writer.write(
               gsl::span{static_cast<const std::byte*>(vari.second->GetBufferPointer()),
                         static_cast<gsl::index>(vari.second->GetBufferSize())});
         }
      }

      // write entry point flag names
      {
         auto flag_names_writer = ep_writer.emplace_child("FLGS"_mn);

         flag_names_writer.write(
            static_cast<std::uint32_t>(entrypoint.second.static_flag_names.size()));

         for (const auto& name : entrypoint.second.static_flag_names) {
            flag_names_writer.write(name);
         }
      }
   }
}

}
