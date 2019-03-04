
#include "vertex_declaration.hpp"
#include "debug_trace.hpp"
#include "helpers.hpp"

#include <algorithm>
#include <array>
#include <map>
#include <utility>
#include <vector>

namespace sp::d3d9 {

namespace {

constexpr std::pair particle_decl_patch =
   {std::array{D3DVERTEXELEMENT9{0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT,
                                 D3DDECLUSAGE_POSITION, 0},
               D3DVERTEXELEMENT9{0, 12, D3DDECLTYPE_D3DCOLOR,
                                 D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
               D3DVERTEXELEMENT9{0, 16, D3DDECLTYPE_SHORT2, D3DDECLMETHOD_DEFAULT,
                                 D3DDECLUSAGE_TEXCOORD, 0}},
    std::array{D3DVERTEXELEMENT9{0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT,
                                 D3DDECLUSAGE_POSITION, 0},
               D3DVERTEXELEMENT9{0, 12, D3DDECLTYPE_D3DCOLOR,
                                 D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
               D3DVERTEXELEMENT9{0, 16, D3DDECLTYPE_SHORT2N, D3DDECLMETHOD_DEFAULT,
                                 D3DDECLUSAGE_TEXCOORD, 0}}};

const std::map<gsl::span<const D3DVERTEXELEMENT9>, gsl::span<const D3DVERTEXELEMENT9>> patchups = {
   {gsl::make_span(particle_decl_patch.first),
    gsl::make_span(particle_decl_patch.second)}};

auto apply_patchups(const gsl::span<const D3DVERTEXELEMENT9> elements) noexcept
   -> gsl::span<const D3DVERTEXELEMENT9>
{
   if (const auto it = patchups.find(elements); it != patchups.end()) {
      return it->second;
   }

   return elements;
}

bool is_compressed_input(const gsl::span<const D3DVERTEXELEMENT9> elements) noexcept
{
   for (const auto& elem : elements) {
      if (is_d3d_decl_type_int(static_cast<D3DDECLTYPE>(elem.Type)))
         return true;
   }

   return false;
}

auto add_missing_tangents(const gsl::span<const D3DVERTEXELEMENT9> input_elements,
                          const bool compressed) noexcept
   -> std::vector<D3DVERTEXELEMENT9>
{
   std::vector<D3DVERTEXELEMENT9> elements{input_elements.cbegin(),
                                           input_elements.cend()};

   auto normal =
      std::find_if(elements.cbegin(), elements.cend(), [](const D3DVERTEXELEMENT9 elem) {
         return elem.Usage == D3DDECLUSAGE_NORMAL && elem.UsageIndex == 0;
      });

   if (normal == elements.cend()) return elements;

   auto next_after_normal = normal + 1;

   if (next_after_normal == elements.cend()) return elements;

   const auto distance = next_after_normal->Offset - normal->Offset;

   if (compressed && distance == 12) {
      auto binormal =
         elements.insert(next_after_normal,
                         {0, static_cast<WORD>(next_after_normal->Offset - 4),
                          D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT,
                          D3DDECLUSAGE_BINORMAL, 0});

      elements.insert(binormal, {0, static_cast<WORD>(binormal->Offset - 4),
                                 D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT,
                                 D3DDECLUSAGE_TANGENT, 0});
   }
   else if (distance == 36) {
      auto binormal =
         elements.insert(next_after_normal,
                         {0, static_cast<WORD>(next_after_normal->Offset - 12),
                          D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT,
                          D3DDECLUSAGE_BINORMAL, 0});

      elements.insert(binormal, {0, static_cast<WORD>(binormal->Offset - 12),
                                 D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT,
                                 D3DDECLUSAGE_TANGENT, 0});
   }

   return elements;
}

auto translate_vertex_elements(const gsl::span<const D3DVERTEXELEMENT9> elements) noexcept
   -> std::vector<core::Input_layout_element>
{
   std::vector<core::Input_layout_element> result;
   result.reserve(elements.size());

   for (const auto& elem : elements) {
      core::Input_layout_element desc{};
      desc.semantic_name =
         d3d_decl_usage_to_string(static_cast<D3DDECLUSAGE>(elem.Usage));
      desc.semantic_index = elem.UsageIndex;
      desc.format = d3d_decl_type_to_dxgi_format(static_cast<D3DDECLTYPE>(elem.Type));
      desc.input_slot = elem.Stream;
      desc.aligned_byte_offset = elem.Offset;

      result.push_back(desc);
   }

   return result;
}

auto create_input_layout(core::Shader_patch& shader_patch,
                         gsl::span<const D3DVERTEXELEMENT9> d3d9_elements) noexcept
   -> core::Game_input_layout
{
   d3d9_elements = apply_patchups(d3d9_elements);
   const bool compressed = is_compressed_input(d3d9_elements);
   const auto elements_with_tangents = add_missing_tangents(d3d9_elements, compressed);

   const auto elements = translate_vertex_elements(elements_with_tangents);

   return shader_patch.create_game_input_layout(elements, compressed);
}

}

Com_ptr<Vertex_declaration> Vertex_declaration::create(
   core::Shader_patch& shader_patch,
   const gsl::span<const D3DVERTEXELEMENT9> elements) noexcept
{
   return Com_ptr{new Vertex_declaration{shader_patch, elements}};
}

HRESULT Vertex_declaration::QueryInterface(const IID& iid, void** object) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!object) return E_INVALIDARG;

   if (iid == IID_IUnknown) {
      *object = static_cast<IUnknown*>(this);
   }
   else if (iid == IID_IDirect3DVertexDeclaration9) {
      *object = this;
   }
   else {
      *object = nullptr;

      return E_NOINTERFACE;
   }

   AddRef();

   return S_OK;
}

ULONG Vertex_declaration::AddRef() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return _ref_count += 1;
}

ULONG Vertex_declaration::Release() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   const auto ref_count = _ref_count -= 1;

   if (ref_count == 0) delete this;

   return ref_count;
}

Vertex_declaration::Vertex_declaration(core::Shader_patch& shader_patch,
                                       const gsl::span<const D3DVERTEXELEMENT9> elements) noexcept
   : _input_layout{create_input_layout(shader_patch, elements)}
{
}

}
