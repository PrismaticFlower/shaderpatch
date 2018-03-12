
#include <array>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <string_view>
#include <system_error>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <d3dcompiler.h>
#include <gsl/gsl>
#include <nlohmann/json.hpp>

#include "checked_invoke.hpp"
#include "com_ptr.hpp"
#include "magic_number.hpp"
#include "ucfb_writer.hpp"

using namespace std::literals;
using namespace sp;

const auto compiler_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_OPTIMIZATION_LEVEL3;

enum Vs_flags : std::uint32_t {
   unskinned = 0, // Unskinned

   light_dir = 16,            // Directional Light
   light_dir_point = 17,      // Directional Lights + Point Light
   light_dir_point2 = 18,     // Directional Lights + 2 Point Lights
   light_dir_point4 = 19,     // Directional Lights + 4 Point Lights
   light_dir_spot = 20,       // Direction Lights + Spot Light
   light_dir_point_spot = 21, // Directional Lights + Point Light + Spot Light
   light_dir_point2_spot = 22, // Directional Lights + 2 Point Lights + Spot Light

   skinned = 64,     // Soft Skinned
   vertexcolor = 128 // Vertexcolor
};

enum Pass_flags : std::uint32_t {
   nolighting = 1,
   lighting = 2,

   notransform = 16,
   position = 32,
   normals = 64,       // position,normals
   binormals = 128,    // position, normals, binormals
   vertex_color = 256, // vertex color
   texcoords = 512     // texcoords
};

struct Vertex_shader_ref {
   Vs_flags flags;
   std::uint32_t index;
};

struct Pass {
   Pass_flags flags;
   std::vector<Vertex_shader_ref> vs_shaders;
   std::uint32_t ps_index;
};

struct State {
   std::uint32_t id{};
   std::vector<Pass> passes;
};

// hash specializations for the compiler's cache
namespace std {

template<>
struct hash<std::tuple<std::string_view, std::string_view, std::pair<std::array<D3D_SHADER_MACRO, 8>, Vs_flags>>> {

   using Type = std::tuple<std::string_view, std::string_view,
                           std::pair<std::array<D3D_SHADER_MACRO, 8>, Vs_flags>>;
   using argument_type = Type;
   using result_type = std::size_t;

   std::size_t operator()(const Type& value) const noexcept
   {
      const auto combine = [](std::size_t& seed, const auto& value) {
         std::hash<std::decay_t<decltype(value)>> hasher{};
         seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      };

      std::size_t seed = 0;

      combine(seed, std::get<0>(value));
      combine(seed, std::get<1>(value));
      combine(seed, std::uint32_t{std::get<2>(value).second});

      for (const auto& define : std::get<2>(value).first) {
         if (define.Name != nullptr) combine(seed, define.Name);
      }

      return seed;
   }
};

template<>
struct hash<std::pair<std::string_view, std::string_view>> {

   using Type = std::pair<std::string_view, std::string_view>;
   using argument_type = Type;
   using result_type = std::size_t;

   std::size_t operator()(const Type& value) const noexcept
   {
      const auto combine = [](std::size_t& seed, const auto& value) {
         std::hash<std::decay_t<decltype(value)>> hasher{};
         seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      };

      std::size_t seed = 0;

      combine(seed, std::get<0>(value));
      combine(seed, std::get<1>(value));

      return seed;
   }
};
}

gsl::span<std::byte> make_span(ID3DBlob& blob) noexcept
{
   return gsl::make_span<std::byte>(static_cast<std::byte*>(blob.GetBufferPointer()),
                                    static_cast<std::ptrdiff_t>(blob.GetBufferSize()));
}

void compile(std::string_view def_path, std::string_view hlsl_path,
             std::string_view out_path);

auto compile_state(const nlohmann::json& state_def, const nlohmann::json& parent_metadata,
                   std::string_view hlsl_path, std::string_view hlsl,
                   std::vector<Com_ptr<ID3DBlob>>& vertex_shaders,
                   std::vector<Com_ptr<ID3DBlob>>& pixel_shaders) -> State;

auto compile_pass(const nlohmann::json& pass_def, const nlohmann::json& parent_metadata,
                  std::string_view hlsl_path, std::string_view hlsl,
                  std::vector<Com_ptr<ID3DBlob>>& vertex_shaders,
                  std::vector<Com_ptr<ID3DBlob>>& pixel_shaders) -> Pass;

// Not thread-safe, uses a static cache to boost compile times and reduce shader
// sizes.
auto compile_vertex_shader(const nlohmann::json& parent_metadata,
                           std::string_view hlsl_path, std::string_view hlsl,
                           std::string_view entry_point, std::string_view target,
                           std::vector<Com_ptr<ID3DBlob>>& vertex_shaders,
                           const std::pair<std::array<D3D_SHADER_MACRO, 8>, Vs_flags>& variation)
   -> std::uint32_t;

// Not thread-safe, uses a static cache to boost compile times and reduce shader
// sizes.
auto compile_pixel_shader(const nlohmann::json& parent_metadata,
                          std::string_view hlsl_path, std::string_view hlsl,
                          std::string_view entry_point, std::string_view target,
                          std::vector<Com_ptr<ID3DBlob>>& pixel_shaders)
   -> std::uint32_t;

void save_shader(std::string_view render_type, std::string_view out_path,
                 const std::vector<Com_ptr<ID3DBlob>>& vertex_shaders,
                 const std::vector<Com_ptr<ID3DBlob>>& pixel_shaders,
                 const std::vector<State>& states);

auto read_file(std::string_view file_name) -> std::string;

auto open_definition(std::string_view def_path) -> nlohmann::json;

auto get_vs_variations(bool skinned, bool lighting, bool vertex_color)
   -> std::vector<std::pair<std::array<D3D_SHADER_MACRO, 8>, Vs_flags>>;

auto get_pass_flags(const nlohmann::json& pass_def) -> Pass_flags;

auto embed_meta_data(const nlohmann::json& extra_metadata,
                     std::string_view hlsl_path, std::string_view entry_point,
                     std::string_view target, std::optional<Vs_flags> flags,
                     Com_ptr<ID3DBlob> shader) -> Com_ptr<ID3DBlob>;

constexpr D3D_SHADER_MACRO operator""_def(const char* chars, const std::size_t) noexcept
{
   return {chars, ""};
}

int main(int arg_count, char* args[])
{
   if (arg_count != 4) {
      std::cout << "usage: <definition path> <hlsl path> <out path>\n";

      return 0;
   }

   try {
      compile(args[1], args[2], args[3]);
   }
   catch (std::exception& e) {
      std::cerr << e.what() << '\n';

      return 1;
   }
}

void compile(std::string_view def_path, std::string_view hlsl_path,
             std::string_view out_path)
{
   const auto hlsl = read_file(hlsl_path);

   const auto definition = open_definition(def_path);

   const std::string render_type = definition["rendertype"];

   const auto metadata = definition.value("metadata", nlohmann::json::object());

   std::vector<Com_ptr<ID3DBlob>> vertex_shaders;
   std::vector<Com_ptr<ID3DBlob>> pixel_shaders;
   std::vector<State> states;

   for (const auto& state_def : definition["states"]) {
      states.emplace_back(compile_state(state_def, metadata, hlsl_path, hlsl,
                                        vertex_shaders, pixel_shaders));
   }

   save_shader(render_type, out_path, vertex_shaders, pixel_shaders, states);
}

auto compile_state(const nlohmann::json& state_def, const nlohmann::json& parent_metadata,
                   std::string_view hlsl_path, std::string_view hlsl,
                   std::vector<Com_ptr<ID3DBlob>>& vertex_shaders,
                   std::vector<Com_ptr<ID3DBlob>>& pixel_shaders) -> State
{
   State state{std::uint32_t{state_def["id"]}};

   auto& passes = state.passes;

   auto metadata = state_def.value("metadata", nlohmann::json::object());
   metadata.update(parent_metadata);

   for (const auto& pass_def : state_def["passes"]) {
      passes.emplace_back(compile_pass(pass_def, metadata, hlsl_path, hlsl,
                                       vertex_shaders, pixel_shaders));
   }

   return state;
}

auto compile_pass(const nlohmann::json& pass_def, const nlohmann::json& parent_metadata,
                  std::string_view hlsl_path, std::string_view hlsl,
                  std::vector<Com_ptr<ID3DBlob>>& vertex_shaders,
                  std::vector<Com_ptr<ID3DBlob>>& pixel_shaders) -> Pass
{
   Pass pass{};

   pass.flags = get_pass_flags(pass_def);

   auto metadata = pass_def.value("metadata", nlohmann::json::object());
   metadata.update(parent_metadata);

   const auto vs_variations =
      get_vs_variations(pass_def["skinned"], pass_def["lighting"],
                        pass_def["vertex_color"]);

   const auto vs_target = pass_def.value("vs_target", "vs_3_0");
   const auto ps_target = pass_def.value("ps_target", "ps_3_0");

   const std::string vs_entry_point = pass_def["vertex_shader"];
   const std::string ps_entry_point = pass_def["pixel_shader"];

   for (const auto& variation : vs_variations) {
      pass.vs_shaders.emplace_back();
      pass.vs_shaders.back().flags = variation.second;
      pass.vs_shaders.back().index =
         compile_vertex_shader(metadata, hlsl_path, hlsl, vs_entry_point,
                               vs_target, vertex_shaders, variation);
   }

   pass.ps_index = compile_pixel_shader(metadata, hlsl_path, hlsl,
                                        ps_entry_point, ps_target, pixel_shaders);

   return pass;
}

auto compile_vertex_shader(const nlohmann::json& parent_metadata,
                           std::string_view hlsl_path, std::string_view hlsl,
                           std::string_view entry_point, std::string_view target,
                           std::vector<Com_ptr<ID3DBlob>>& vertex_shaders,
                           const std::pair<std::array<D3D_SHADER_MACRO, 8>, Vs_flags>& variation)
   -> std::uint32_t
{
   using Cache_key_tuple =
      std::tuple<std::string_view, std::string_view,
                 std::pair<std::array<D3D_SHADER_MACRO, 8>, Vs_flags>>;

   static std::unordered_map<std::uint32_t, std::uint32_t> cache;

   const auto key =
      std::hash<Cache_key_tuple>{}(std::make_tuple(entry_point, target, variation));

   const auto cached = cache.find(key);

   if (cached != std::cend(cache)) return cached->second;

   cache[key] = static_cast<std::uint32_t>(vertex_shaders.size());

   Com_ptr<ID3DBlob> error_message;
   Com_ptr<ID3DBlob> shader;

   auto result =
      D3DCompile(hlsl.data(), hlsl.length(), hlsl_path.data(),
                 variation.first.data(), D3D_COMPILE_STANDARD_FILE_INCLUDE,
                 entry_point.data(), target.data(), compiler_flags, NULL,
                 shader.clear_and_assign(), error_message.clear_and_assign());

   if (result != S_OK) {
      throw std::runtime_error{static_cast<char*>(error_message->GetBufferPointer())};
   }
   else if (error_message) {
      std::cout << static_cast<char*>(error_message->GetBufferPointer());
   }

   shader = embed_meta_data(parent_metadata, hlsl_path, entry_point, target,
                            variation.second, std::move(shader));

   vertex_shaders.emplace_back(std::move(shader));

   return cache[key];
}

auto compile_pixel_shader(const nlohmann::json& parent_metadata,
                          std::string_view hlsl_path, std::string_view hlsl,
                          std::string_view entry_point, std::string_view target,
                          std::vector<Com_ptr<ID3DBlob>>& pixel_shaders) -> std::uint32_t
{
   using Cache_key_pair = std::pair<std::string_view, std::string_view>;

   static std::unordered_map<std::uint32_t, std::uint32_t> cache;

   const auto key = std::hash<Cache_key_pair>{}(std::make_pair(entry_point, target));

   const auto cached = cache.find(key);

   if (cached != std::cend(cache)) return cached->second;

   cache[key] = static_cast<std::uint32_t>(pixel_shaders.size());

   Com_ptr<ID3DBlob> error_message;
   Com_ptr<ID3DBlob> shader;

   auto result =
      D3DCompile(hlsl.data(), hlsl.length(), hlsl_path.data(), nullptr,
                 D3D_COMPILE_STANDARD_FILE_INCLUDE, entry_point.data(),
                 target.data(), compiler_flags, NULL, shader.clear_and_assign(),
                 error_message.clear_and_assign());

   if (result != S_OK) {
      throw std::runtime_error{static_cast<char*>(error_message->GetBufferPointer())};
   }
   else if (error_message) {
      std::cout << static_cast<char*>(error_message->GetBufferPointer());
   }

   shader = embed_meta_data(parent_metadata, hlsl_path, entry_point, target,
                            std::nullopt, std::move(shader));

   pixel_shaders.emplace_back(std::move(shader));

   return cache[key];
}

void save_shader(std::string_view render_type, std::string_view out_path,
                 const std::vector<Com_ptr<ID3DBlob>>& vertex_shaders,
                 const std::vector<Com_ptr<ID3DBlob>>& pixel_shaders,
                 const std::vector<State>& states)
{
   Ucfb_writer writer{out_path};

   auto shdr = writer.emplace_child("SHDR"_mn);

   shdr.emplace_child("RTYP"_mn).write(render_type);
   shdr.emplace_child("NAME"_mn).write(render_type);
   shdr.emplace_child("INFO"_mn).write(std::uint32_t{1u}, std::uint8_t{26u});

   auto pipe = shdr.emplace_child("PIPE"_mn);
   pipe.emplace_child("INFO"_mn)
      .write(std::uint32_t{1u}, static_cast<std::uint32_t>(states.size()),
             static_cast<std::uint32_t>(vertex_shaders.size()),
             static_cast<std::uint32_t>(pixel_shaders.size()));

   auto vsp_ = pipe.emplace_child("VSP_"_mn);

   for (const auto& shader : vertex_shaders) {
      auto vs__ = vsp_.emplace_child("VS__"_mn);

      vs__.write(static_cast<std::uint32_t>(shader->GetBufferSize()));
      vs__.write(make_span(*shader));
   }

   auto& psp_ = pipe.emplace_child("PSP_"_mn);

   for (const auto& shader : pixel_shaders) {
      auto ps__ = psp_.emplace_child("PS__"_mn);

      ps__.write(static_cast<std::uint32_t>(shader->GetBufferSize()));
      ps__.write(make_span(*shader));
   }

   for (std::uint32_t i = 0u; i < states.size(); ++i) {
      const auto& state = states[i];

      auto stat = pipe.emplace_child("STAT"_mn);
      stat.emplace_child("INFO"_mn).write(i, static_cast<std::uint32_t>(
                                                state.passes.size()));

      for (auto j = 0ui32; j < state.passes.size(); ++j) {
         const auto& pass = state.passes[j];

         auto pass_chunk = stat.emplace_child("PASS"_mn);
         pass_chunk.emplace_child("INFO"_mn).write(pass.flags);

         for (const auto& vs_ref : pass.vs_shaders) {
            pass_chunk.emplace_child("PVS_"_mn).write(vs_ref.flags, vs_ref.index);
         }

         pass_chunk.emplace_child("PPS_"_mn).write(pass.ps_index);
      }
   }
}

auto read_file(std::string_view file_name) -> std::string
{
   return {std::istreambuf_iterator<char>(std::ifstream(file_name.data())),
           std::istreambuf_iterator<char>()};
}

auto open_definition(std::string_view def_path) -> nlohmann::json
{
   std::ifstream file{def_path.data()};

   nlohmann::json config;
   file >> config;

   return config;
}

auto get_vs_variations(bool skinned, bool lighting, bool vertex_color)
   -> std::vector<std::pair<std::array<D3D_SHADER_MACRO, 8>, Vs_flags>>
{
   std::vector<std::pair<std::array<D3D_SHADER_MACRO, 8>, Vs_flags>> variations;
   variations.reserve(32);

   const auto add_variation = [&](auto flags, std::array<D3D_SHADER_MACRO, 8> defines) {
      variations.emplace_back();
      variations.back().first = defines;
      variations.back().second = static_cast<Vs_flags>(flags);
   };

   const auto nulldef = D3D_SHADER_MACRO{nullptr, nullptr};
   const auto unskinned_def = "TRANSFORM_UNSKINNED"_def;
   const auto skinned_def = "TRANSFORM_SOFT_SKINNED"_def;
   const auto color_def = "USE_VERTEX_COLOR"_def;

   const auto light_dir_def = "LIGHTING_DIRECTIONAL"_def;
   const auto light_point_def = "LIGHTING_POINT_0"_def;
   const auto light_point_1_def = "LIGHTING_POINT_1"_def;
   const auto light_point_23_def = "LIGHTING_POINT_23"_def;
   const auto light_spot_def = "LIGHTING_SPOT_0"_def;

   // Unskinned
   add_variation(unskinned, {unskinned_def, nulldef});

   // Unskinned Vertex colored
   if (vertex_color) {
      add_variation(unskinned | vertexcolor, {unskinned_def, color_def, nulldef});
   }

   // Skinned Defintions
   if (skinned) {
      // Skinned
      add_variation(Vs_flags::skinned, {skinned_def, nulldef});
   }

   // Skinned Vertex colored
   if (skinned && vertex_color) {
      add_variation(Vs_flags::skinned | vertexcolor, {skinned_def, color_def, nulldef});
   }

   // Unskinned Lighting Definitions
   if (lighting) {
      // Directional Light
      add_variation(unskinned | light_dir, {unskinned_def, light_dir_def, nulldef});

      // Directional Lights + Point Light
      add_variation(unskinned | light_dir_point,
                    {unskinned_def, light_dir_def, light_point_def, nulldef});

      // Directional Lights + 2 Point Lights
      add_variation(unskinned | light_dir_point2,
                    {unskinned_def, light_dir_def, light_point_def,
                     light_point_1_def, nulldef});

      // Directional Lights + 4 Point Lights
      add_variation(unskinned | light_dir_point4,
                    {unskinned_def, light_dir_def, light_point_def,
                     light_point_1_def, light_point_23_def, nulldef});

      // Direction Lights + Spot Light
      add_variation(unskinned | light_dir_spot,
                    {unskinned_def, light_dir_def, light_spot_def, nulldef});

      // Directional Lights + Point Light + Spot Light
      add_variation(unskinned | light_dir_point_spot,
                    {unskinned_def, light_dir_def, light_point_def,
                     light_spot_def, nulldef});

      // Directional Lights + 2 Point Lights + Spot Light
      add_variation(unskinned | light_dir_point2_spot,
                    {unskinned_def, light_dir_def, light_point_def,
                     light_spot_def, light_point_1_def, nulldef});
   }

   // Unskinned Lighting Vertexcolor Definitions
   if (lighting && vertex_color) {
      // Directional Light
      add_variation(unskinned | vertexcolor | light_dir,
                    {unskinned_def, color_def, light_dir_def, nulldef});

      // Directional Lights + Point Light
      add_variation(unskinned | vertexcolor | light_dir_point,
                    {unskinned_def, color_def, light_dir_def, light_point_def, nulldef});

      // Directional Lights + 2 Point Lights
      add_variation(unskinned | vertexcolor | light_dir_point2,
                    {unskinned_def, color_def, light_dir_def, light_point_def,
                     light_point_1_def, nulldef});

      // Directional Lights + 4 Point Lights
      add_variation(unskinned | vertexcolor | light_dir_point4,
                    {unskinned_def, color_def, light_dir_def, light_point_def,
                     light_point_1_def, light_point_23_def, nulldef});

      // Direction Lights + Spot Light
      add_variation(unskinned | vertexcolor | light_dir_spot,
                    {unskinned_def, color_def, light_dir_def, light_spot_def, nulldef});

      // Directional Lights + Point Light + Spot Light
      add_variation(unskinned | vertexcolor | light_dir_point_spot,
                    {unskinned_def, color_def, light_dir_def, light_point_def,
                     light_spot_def, nulldef});

      // Directional Lights + 2 Point Lights + Spot Light
      add_variation(unskinned | vertexcolor | light_dir_point2_spot,
                    {unskinned_def, color_def, light_dir_def, light_point_def,
                     light_spot_def, light_point_1_def, nulldef});
   }

   // Skinned Lighting Definitions
   if (lighting && skinned) {
      // Directional Light
      add_variation(Vs_flags::skinned | light_dir,
                    {skinned_def, light_dir_def, nulldef});

      // Directional Lights + Point Light
      add_variation(Vs_flags::skinned | light_dir_point,
                    {skinned_def, light_dir_def, light_point_def, nulldef});

      // Directional Lights + 2 Point Lights
      add_variation(Vs_flags::skinned | light_dir_point2,
                    {skinned_def, light_dir_def, light_point_def,
                     light_point_1_def, nulldef});

      // Directional Lights + 4 Point Lights
      add_variation(Vs_flags::skinned | light_dir_point4,
                    {skinned_def, light_dir_def, light_point_def,
                     light_point_1_def, light_point_23_def, nulldef});

      // Direction Lights + Spot Light
      add_variation(Vs_flags::skinned | light_dir_spot,
                    {skinned_def, light_dir_def, light_spot_def, nulldef});

      // Directional Lights + Point Light + Spot Light
      add_variation(Vs_flags::skinned | light_dir_point_spot,
                    {skinned_def, light_dir_def, light_point_def,
                     light_spot_def, nulldef});

      // Directional Lights + 2 Point Lights + Spot Light
      add_variation(Vs_flags::skinned | light_dir_point2_spot,
                    {skinned_def, light_dir_def, light_point_def,
                     light_spot_def, light_point_1_def, nulldef});
   }

   // Skinned Lighting Vertexcolor Definitions
   if (lighting && skinned && vertex_color) {
      // Directional Light
      add_variation(Vs_flags::skinned | vertexcolor | light_dir,
                    {skinned_def, color_def, light_dir_def, nulldef});

      // Directional Lights + Point Light
      add_variation(Vs_flags::skinned | vertexcolor | light_dir_point,
                    {skinned_def, color_def, light_dir_def, light_point_def, nulldef});

      // Directional Lights + 2 Point Lights
      add_variation(Vs_flags::skinned | vertexcolor | light_dir_point2,
                    {skinned_def, color_def, light_dir_def, light_point_def,
                     light_point_1_def, nulldef});

      // Directional Lights + 4 Point Lights
      add_variation(Vs_flags::skinned | vertexcolor | light_dir_point4,
                    {skinned_def, color_def, light_dir_def, light_point_def,
                     light_point_1_def, light_point_23_def, nulldef});

      // Direction Lights + Spot Light
      add_variation(Vs_flags::skinned | vertexcolor | light_dir_spot,
                    {skinned_def, color_def, light_dir_def, light_spot_def, nulldef});

      // Directional Lights + Point Light + Spot Light
      add_variation(Vs_flags::skinned | vertexcolor | light_dir_point_spot,
                    {skinned_def, color_def, light_dir_def, light_point_def,
                     light_spot_def, nulldef});

      // Directional Lights + 2 Point Lights + Spot Light
      add_variation(Vs_flags::skinned | vertexcolor | light_dir_point2_spot,
                    {skinned_def, color_def, light_dir_def, light_point_def,
                     light_spot_def, light_point_1_def, nulldef});
   }

   return variations;
}

auto get_pass_flags(const nlohmann::json& pass_def) -> Pass_flags
{
   Pass_flags flags{0};

   const std::string transform = pass_def["transform"];

   const auto set_flag = [&flags](Pass_flags flag) {
      flags = static_cast<Pass_flags>(flags | flag);
   };

   if (pass_def["lighting"]) {
      set_flag(Pass_flags::lighting);
   }
   else {
      set_flag(Pass_flags::nolighting);
   }

   if (transform == "none"sv) {
      set_flag(Pass_flags::notransform);

      return flags;
   }
   else if (transform == "position"sv) {
      set_flag(Pass_flags::position);
   }
   else if (transform == "normals"sv) {
      set_flag(Pass_flags::normals);
   }
   else if (transform == "binormals"sv) {
      set_flag(Pass_flags::binormals);
   }
   else {
      throw std::runtime_error{"Invalid transform value: "s + transform};
   }

   if (pass_def["vertex_color"]) set_flag(Pass_flags::vertex_color);
   if (pass_def["texture_coords"]) set_flag(Pass_flags::texcoords);

   return flags;
}

auto embed_meta_data(const nlohmann::json& extra_metadata,
                     std::string_view hlsl_path, std::string_view entry_point,
                     std::string_view target, std::optional<Vs_flags> flags,
                     Com_ptr<ID3DBlob> shader) -> Com_ptr<ID3DBlob>
{
   using Path = std::experimental::filesystem::path;

   const Path path{std::cbegin(hlsl_path), std::cend(hlsl_path)};

   nlohmann::json meta = extra_metadata;

   meta["name"s] = path.stem().u8string();
   meta["entry_point"s] = entry_point.data();
   meta["target"s] = target.data();

   if (flags) meta["vs_flags"s] = static_cast<std::uint32_t>(*flags);

   auto meta_buffer = nlohmann::json::to_msgpack(meta);

   const auto bytecode_words = static_cast<DWORD*>(shader->GetBufferPointer());
   const auto bytecode_size = shader->GetBufferSize() / sizeof(DWORD);

   const auto meta_size =
      (meta_buffer.size() / sizeof(DWORD)) + (meta_buffer.size() % sizeof(DWORD));

   if (meta_size >= 100000) {
      throw std::runtime_error{
         "Shader meta data too large to embed in bytecode."};
   }

   std::vector<DWORD> bytecode;
   bytecode.reserve(bytecode_size + 3 + meta_size);

   bytecode.emplace_back(bytecode_words[0]);

   // comment 0xFFFE
   bytecode.emplace_back((0xFFFEu) | ((meta_size + 2u) << 16u));
   bytecode.emplace_back(static_cast<DWORD>("META"_mn));
   bytecode.emplace_back(static_cast<DWORD>(meta_buffer.size()));

   const auto meta_start = bytecode.size();

   bytecode.resize(bytecode.size() + meta_size);

   std::memcpy(&bytecode[meta_start], meta_buffer.data(), meta_buffer.size());

   for (auto i = 1ui32; i < bytecode_size; ++i) {
      bytecode.emplace_back(bytecode_words[i]);
   }

   Com_ptr<ID3DBlob> new_shader;

   const auto result =
      D3DCreateBlob(bytecode.size() * 4, new_shader.clear_and_assign());

   if (result != S_OK) throw std::system_error{result, std::system_category()};

   std::memcpy(new_shader->GetBufferPointer(), bytecode.data(), bytecode.size() * 4);

   return new_shader;
}
