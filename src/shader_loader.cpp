
#include "shader_loader.hpp"
#include "logger.hpp"

#include <string>
#include <vector>

namespace sp {

using namespace std::literals;

namespace {

template<Magic_number mn>
auto read_shaders(ucfb::Reader_strict<mn> reader, IDirect3DDevice9& device,
                  std::string_view rendertype)
{
   static_assert(mn == "VSHD"_mn || mn == "PSHD"_mn);
   using Type =
      std::conditional_t<mn == "VSHD"_mn, IDirect3DVertexShader9, IDirect3DPixelShader9>;

   const gsl::index count =
      reader.read_child_strict<"INFO"_mn>().read_trivial<std::uint32_t>();

   std::vector<Com_ptr<Type>> shaders;
   shaders.reserve(count);

   for (gsl::index i = 0; i < count; ++i) {
      constexpr auto bc_mn = (mn == "VSHD"_mn) ? "VSBC"_mn : "PSBC"_mn;

      auto bc_reader = reader.read_child_strict<bc_mn>();
      const auto bytecode = bc_reader.read_array<DWORD>(bc_reader.size() / 4);

      HRESULT result{E_FAIL};
      shaders.emplace_back();

      if constexpr (mn == "VSHD"_mn) {
         result = device.CreateVertexShader(bytecode.data(),
                                            shaders.back().clear_and_assign());
      }
      else if (mn == "PSHD"_mn) {
         result = device.CreatePixelShader(bytecode.data(),
                                           shaders.back().clear_and_assign());
      }

      if (FAILED(result)) {
         throw std::runtime_error{
            "Direct3D rejected bytecode for shader in rendertype: "s += rendertype};
      }
   }

   return shaders;
}
}

auto load_shader(ucfb::Reader reader, IDirect3DDevice9& device)
   -> std::pair<std::string, Shader_group>
{
   const auto rendertype = reader.read_child_strict<"RTYP"_mn>().read_string();

   const auto vertex_shaders =
      read_shaders(reader.read_child_strict<"VSHD"_mn>(), device, rendertype);
   const auto pixel_shaders =
      read_shaders(reader.read_child_strict<"PSHD"_mn>(), device, rendertype);

   auto shader_refs_reader = reader.read_child_strict<"SHRS"_mn>();

   Shader_group group;

   while (shader_refs_reader) {
      auto shader_reader = shader_refs_reader.read_child_strict<"SHDR"_mn>();

      const auto name = shader_reader.read_child_strict<"NAME"_mn>().read_string();
      const gsl::index variation_count =
         shader_reader.read_child_strict<"INFO"_mn>().read_trivial<std::uint32_t>();

      Shader_variations variations;

      for (gsl::index i = 0; i < variation_count; ++i) {
         auto variation_reader = shader_reader.read_child_strict<"VARI"_mn>();

         const auto flags = variation_reader.read_trivial<Shader_flags>();
         const auto vs_ref = variation_reader.read_trivial<std::uint32_t>();
         const auto ps_ref = variation_reader.read_trivial<std::uint32_t>();

         variations.set(flags, {vertex_shaders.at(vs_ref), pixel_shaders.at(ps_ref)});
      }

      group.add(std::string{name}, std::move(variations));
   }

   return {std::string{rendertype}, group};
}
}
