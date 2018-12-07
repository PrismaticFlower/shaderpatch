
#include "vertex_declaration.hpp"
#include "debug_trace.hpp"
#include "helpers.hpp"

#include <array>
#include <map>
#include <utility>

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

auto apply_patchups(const gsl::span<const D3DVERTEXELEMENT9> elements)
   -> gsl::span<const D3DVERTEXELEMENT9>
{
   if (const auto it = patchups.find(elements); it != patchups.end()) {
      return it->second;
   }

   return elements;
}

auto translate_vertex_elements(const gsl::span<const D3DVERTEXELEMENT9> elements)
   -> std::vector<D3D11_INPUT_ELEMENT_DESC>
{
   std::vector<D3D11_INPUT_ELEMENT_DESC> result;
   result.reserve(elements.size());

   for (const auto& elem : elements) {
      D3D11_INPUT_ELEMENT_DESC desc{};
      desc.SemanticName =
         d3d_decl_usage_to_cstr(static_cast<D3DDECLUSAGE>(elem.Usage));
      desc.SemanticIndex = elem.UsageIndex;
      desc.Format = d3d_decl_type_to_dxgi_format(static_cast<D3DDECLTYPE>(elem.Type));
      desc.InputSlot = elem.Stream;
      desc.AlignedByteOffset = elem.Offset;

      result.push_back(desc);
   }

   return result;
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
   : _input_layout{shader_patch.create_game_input_layout(
        translate_vertex_elements(apply_patchups(elements)))},
     _layout_desc{translate_vertex_elements(apply_patchups(elements))}
{
}

}
