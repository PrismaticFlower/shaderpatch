
#include "declaration_munge.hpp"
#include "compiler_helpers.hpp"
#include "compose_exception.hpp"
#include "file_helpers.hpp"
#include "magic_number.hpp"
#include "req_file_helpers.hpp"
#include "shader_metadata.hpp"
#include "shader_variations.hpp"
#include "synced_io.hpp"
#include "ucfb_writer.hpp"

#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>

#include <gsl/gsl>

using namespace std::literals;

namespace fs = std::filesystem;

namespace sp {

Declaration_munge::Declaration_munge(const fs::path& declaration_path,
                                     const fs::path& output_dir)
{
   Expects(fs::is_regular_file(declaration_path) && fs::is_directory(output_dir));

   const auto output_path =
      output_dir / declaration_path.stem().replace_extension(".shader_declaration"s);

   if (fs::exists(output_path) &&
       (fs::last_write_time(declaration_path) < fs::last_write_time(output_path))) {
      return;
   }

   synced_print("Munging shader declaration "sv,
                declaration_path.filename().string(), "..."sv);

   const auto declaration = read_json_file(declaration_path);

   _rendertype_name = declaration["rendertype"].get<std::string>();
   _rendertype = rendertype_from_string(_rendertype_name);

   const auto srgb_state = declaration.value("srgb_state", std::array<bool, 4>{});

   for (const auto& state_def : declaration["states"]) {
      _states.emplace_back(munge_state(state_def, srgb_state));
   }

   save(output_path, declaration_path.filename().u8string());
}

void Declaration_munge::save(const fs::path& output_path, const std::string& input_filename)
{
   auto file = ucfb::open_file_for_output(output_path.string());

   ucfb::Writer writer{file};

   auto shdr = writer.emplace_child("SHDR"_mn);

   shdr.emplace_child("RTYP"_mn).write(_rendertype_name);
   shdr.emplace_child("NAME"_mn).write(input_filename);
   shdr.emplace_child("INFO"_mn).write(std::uint32_t{1u}, std::uint8_t{26u});

   // write pipeline
   {
      auto pipe = shdr.emplace_child("PIPE"_mn);
      pipe.emplace_child("INFO"_mn).write(std::uint32_t{1u},
                                          static_cast<std::uint32_t>(_states.size()),
                                          static_cast<std::uint32_t>(_shaders.size()),
                                          std::uint32_t{1u});

      // write vertex shaders
      {
         auto vsp_ = pipe.emplace_child("VSP_"_mn);

         for (const auto& shader : _shaders) {
            auto vs__ = vsp_.emplace_child("VS__"_mn);

            const auto data = serialize_shader_metadata(shader);

            vs__.write(static_cast<std::uint32_t>(gsl::make_span(data).size_bytes()));
            vs__.write(gsl::make_span(data));
         }
      }

      // write pixel shaders
      pipe.emplace_child("PSP_"_mn).emplace_child("PS__"_mn).write("NULL"_mn);

      for (std::uint32_t i = 0u; i < _states.size(); ++i) {
         const auto& state = _states[i];

         auto stat = pipe.emplace_child("STAT"_mn);
         stat.emplace_child("INFO"_mn).write(i, static_cast<std::uint32_t>(
                                                   state.passes.size()));

         for (std::uint32_t j = 0u; j < state.passes.size(); ++j) {
            const auto& pass = state.passes[j];

            auto pass_chunk = stat.emplace_child("PASS"_mn);
            pass_chunk.emplace_child("INFO"_mn).write(pass.flags);

            for (const auto& ref : pass.shader_refs) {
               pass_chunk.emplace_child("PVS_"_mn).write(ref.flags, ref.index);
            }

            pass_chunk.emplace_child("PPS_"_mn).write(0);
         }
      }
   }
}

auto Declaration_munge::munge_state(const nlohmann::json& state_def,
                                    std::array<bool, 4> srgb_state)
   -> Declaration_munge::State
{
   State state{std::uint32_t{state_def["id"]}};

   srgb_state = state_def.value("srgb_state", srgb_state);

   for (const auto& pass_def : state_def["passes"]) {
      state.passes.emplace_back(munge_pass(pass_def, srgb_state));
   }

   return state;
}

auto Declaration_munge::munge_pass(const nlohmann::json& pass_def,
                                   std::array<bool, 4> srgb_state)
   -> Declaration_munge::Pass
{
   Pass pass{};

   pass.flags =
      get_pass_flags(pass_def["base_input"s], pass_def["lighting"],
                     pass_def["vertex_color"s], pass_def["texture_coords"s]);

   srgb_state = pass_def.value("srgb_state", srgb_state);

   for (const auto& variation :
        shader::get_declaration_shader_variations(pass.flags,
                                                  pass_def["skinned"])) {
      Shader_metadata shader;

      shader.rendertype = _rendertype;
      shader.rendertype_name = _rendertype_name;
      shader.shader_name = pass_def.at("name"s).get<std::string>();
      shader.srgb_state = srgb_state;

      Shader_ref shader_ref;

      std::tie(shader_ref.flags, shader.vertex_shader_flags,
               shader.pixel_shader_flags) = variation;

      shader_ref.index = static_cast<std::uint32_t>(_shaders.size());

      _shaders.emplace_back(shader);
      pass.shader_refs.emplace_back(shader_ref);
   }

   return pass;
}

}
