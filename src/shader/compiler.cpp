
#include "compiler.hpp"
#include "../logger.hpp"
#include "retry_dialog.hpp"

#include <type_traits>

#include <absl/container/inlined_vector.h>

#include <d3dcompiler.h>

using namespace std::literals;

namespace sp::shader {

namespace {

using Shader_defines = absl::InlinedVector<D3D_SHADER_MACRO, 64>;

class Includer : public ID3DInclude {
public:
   explicit Includer(const Source_file_store& file_store)
      : _file_store{file_store}
   {
   }

   auto __stdcall Open([[maybe_unused]] D3D_INCLUDE_TYPE include_type,
                       LPCSTR file_name, [[maybe_unused]] LPCVOID parent_data,
                       LPCVOID* data_out, UINT* data_size_out) noexcept -> HRESULT override
   {
      auto data = _file_store.data(file_name);

      if (!data) return E_FAIL;

      static_assert(std::is_same_v<std::size_t, UINT>);

      *data_out = data->data();
      *data_size_out = data->size();

      return S_OK;
   }

   auto __stdcall Close(LPCVOID) noexcept -> HRESULT override
   {
      return S_OK;
   }

private:
   const Source_file_store& _file_store;
};

auto get_stage_define(const Entrypoint_description& entrypoint) -> D3D_SHADER_MACRO
{
   switch (entrypoint.stage) {
      // clang-format off
   case Stage::compute:  return {"__COMPUTE_SHADER__", "1"};
   case Stage::vertex:   return {"__VERTEX_SHADER__", "1"};
   case Stage::hull:     return {"__HULL_SHADER__", "1"};
   case Stage::domain:   return {"__DOMAIN_SHADER__", "1"};
   case Stage::geometry: return {"__GEOMETRY_SHADER__", "1"};
   case Stage::pixel:    return {"__PIXEL_SHADER__", "1"};
      // clang-format on
   }

   std::terminate();
}

void get_vertex_shader_defines(const Entrypoint_description& entrypoint,
                               const Vertex_shader_flags vertex_shader_flags,
                               Shader_defines& output)
{
   const auto input_state = entrypoint.vertex_state.generic_input_state;

   const bool hard_skinned =
      ((vertex_shader_flags & Vertex_shader_flags::hard_skinned) !=
       Vertex_shader_flags::none) &&
      input_state.skinned;
   const bool color = ((vertex_shader_flags & Vertex_shader_flags::color) !=
                       Vertex_shader_flags::none) &&
                      input_state.color;

   if (input_state.position) {
      output.emplace_back("__VERTEX_INPUT_POSITION__", "1");
   }

   if (hard_skinned) {
      output.emplace_back("__VERTEX_INPUT_BLEND_WEIGHT__", "1");
      output.emplace_back("__VERTEX_INPUT_BLEND_INDICES__", "1");
   }

   if (input_state.normal) {
      output.emplace_back("__VERTEX_INPUT_NORMAL__", "1");
   }

   if (input_state.tangents) {
      output.emplace_back("__VERTEX_INPUT_TANGENTS__", "1");
   }

   if (color) {
      output.emplace_back("__VERTEX_INPUT_COLOR__", "1");
   }

   if (input_state.texture_coords) {
      output.emplace_back("__VERTEX_INPUT_TEXCOORDS__", "1");
   }

   output.emplace_back(hard_skinned ? "__VERTEX_TRANSFORM_HARD_SKINNED__"
                                    : "__VERTEX_TRANSFORM_UNSKINNED__",
                       "1");
}

auto get_shader_defines(const Entrypoint_description& entrypoint,
                        const std::uint64_t static_flags,
                        const Vertex_shader_flags vertex_shader_flags) -> Shader_defines
{
   Shader_defines d3d_shader_defines;

   d3d_shader_defines.emplace_back(get_stage_define(entrypoint));

   if (entrypoint.stage == Stage::vertex) {
      get_vertex_shader_defines(entrypoint, vertex_shader_flags, d3d_shader_defines);
   }

   entrypoint.static_flags.get_flags_defines(static_flags, d3d_shader_defines);

   for (const auto& define : entrypoint.preprocessor_defines) {
      d3d_shader_defines.emplace_back(define.name.data(), define.definition.data());
   }

   d3d_shader_defines.emplace_back(nullptr, nullptr);

   return d3d_shader_defines;
}

auto get_target(const Entrypoint_description& entrypoint) -> const char*
{
   switch (entrypoint.stage) {
      // clang-format off
   case Stage::compute:  return "cs_5_0";
   case Stage::vertex:   return "vs_5_0";
   case Stage::hull:     return "hs_5_0";
   case Stage::domain:   return "ds_5_0";
   case Stage::geometry: return "gs_5_0";
   case Stage::pixel:    return "ps_5_0";
      // clang-format on
   }

   std::terminate();
}

}

auto compile(Source_file_store& file_store, const Entrypoint_description& entrypoint,
             const std::uint64_t static_flags,
             const Vertex_shader_flags vertex_shader_flags) noexcept -> Bytecode_blob
{
   auto source = file_store.data(entrypoint.source_name);

   if (!source) {
      log_and_terminate("Unable to get shader source code. Can't compile.");
   }

   auto shader_defines =
      get_shader_defines(entrypoint, static_flags, vertex_shader_flags);

   Includer includer{file_store};
   Com_ptr<ID3DBlob> bytecode_result;
   Com_ptr<ID3DBlob> error_messages;

   auto result =
      D3DCompile(source->data(), source->size(),
                 file_store.file_path(entrypoint.source_name).data(),
                 shader_defines.data(), &includer,
                 entrypoint.function_name.data(), get_target(entrypoint),
                 D3DCOMPILE_OPTIMIZATION_LEVEL2 | D3DCOMPILE_WARNINGS_ARE_ERRORS,
                 0, bytecode_result.clear_and_assign(),
                 error_messages.clear_and_assign());

   if (FAILED(result)) {
      const std::string_view error_message_view = {static_cast<const char*>(
                                                      error_messages->GetBufferPointer()),
                                                   error_messages->GetBufferSize()};

      if (retry_dialog("Shader Compile Error"s, error_message_view)) {
         file_store.reload();

         return compile(file_store, entrypoint, static_flags, vertex_shader_flags);
      }

      log_and_terminate("Unable to compile shader!\n", error_message_view);
   }

   log_debug("Compiled shader {}:{}({:x})"sv, entrypoint.source_name,
             entrypoint.function_name, static_flags);

   return Bytecode_blob{std::move(bytecode_result)};
}
}
