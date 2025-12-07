
#include "vertex_declaration.hpp"
#include "debug_trace.hpp"
#include "helpers.hpp"

#include <algorithm>
#include <array>
#include <utility>
#include <vector>

namespace sp::d3d9 {

namespace {

void add_missing_tangents(std::vector<D3DVERTEXELEMENT9>& elements) noexcept
{
   auto normal =
      std::find_if(elements.cbegin(), elements.cend(), [](const D3DVERTEXELEMENT9 elem) {
         return elem.Usage == D3DDECLUSAGE_NORMAL && elem.UsageIndex == 0;
      });

   if (normal == elements.cend()) return;

   auto next_after_normal = normal + 1;

   if (next_after_normal == elements.cend()) return;

   const auto distance = next_after_normal->Offset - normal->Offset;

   if (distance == 12) {
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
}

auto create_input_layout(core::Shader_patch& shader_patch,
                         std::span<const D3DVERTEXELEMENT9> d3d9_elements) noexcept
   -> core::Game_input_layout
{
   std::vector<D3DVERTEXELEMENT9> patched_d3d9_elements{d3d9_elements.begin(),
                                                        d3d9_elements.end()};

   add_missing_tangents(patched_d3d9_elements);

   std::vector<core::Input_layout_element> elements;
   elements.reserve(patched_d3d9_elements.size());

   bool compressed_position = false;
   bool compressed_texcoords = false;
   bool has_vertex_weights = false;

   for (const auto& elem : patched_d3d9_elements) {
      core::Input_layout_element desc{};

      desc.semantic_name =
         d3d_decl_usage_to_string(static_cast<D3DDECLUSAGE>(elem.Usage));
      desc.semantic_index = elem.UsageIndex;
      desc.input_slot = elem.Stream;
      desc.aligned_byte_offset = elem.Offset;

      bool is_valid = true;

      switch (elem.Usage) {
      case D3DDECLUSAGE_POSITION: {
         switch (elem.Type) {
         case D3DDECLTYPE_FLOAT1: {
            desc.format = DXGI_FORMAT_R32_FLOAT;
         } break;
         case D3DDECLTYPE_FLOAT2: {
            desc.format = DXGI_FORMAT_R32G32_FLOAT;
         } break;
         case D3DDECLTYPE_FLOAT3: {
            desc.format = DXGI_FORMAT_R32G32B32_FLOAT;
         } break;
         case D3DDECLTYPE_FLOAT4: {
            desc.format = DXGI_FORMAT_R32G32B32A32_FLOAT;
         } break;
         case D3DDECLTYPE_SHORT2: {
            desc.format = DXGI_FORMAT_R16G16_SINT;
         } break;
         case D3DDECLTYPE_SHORT4: {
            desc.format = DXGI_FORMAT_R16G16B16A16_SINT;

            compressed_position = true;
         } break;
         default: {
            is_valid = false;

            log_fmt(Log_level::warning,
                    "Unknown or unsupported D3DDECLTYPE ('{}') for usagae '{}' "
                    "encountered. "
                    "Rendering maybe buggy or SP may even crash.",
                    elem.Type, desc.semantic_name);
         } break;
         }
      } break;
      case D3DDECLUSAGE_BLENDWEIGHT: {
         has_vertex_weights = true;

         switch (elem.Type) {
         case D3DDECLTYPE_FLOAT1: {
            desc.format = DXGI_FORMAT_R32_FLOAT;
         } break;
         case D3DDECLTYPE_FLOAT2: {
            desc.format = DXGI_FORMAT_R32G32_FLOAT;
         } break;
         case D3DDECLTYPE_FLOAT3: {
            desc.format = DXGI_FORMAT_R32G32B32_FLOAT;
         } break;
         case D3DDECLTYPE_FLOAT4: {
            desc.format = DXGI_FORMAT_R32G32B32A32_FLOAT;
         } break;
         case D3DDECLTYPE_D3DCOLOR: {
            desc.format = DXGI_FORMAT_B8G8R8A8_UNORM;
         } break;
         default: {
            is_valid = false;

            log_fmt(Log_level::warning,
                    "Unknown or unsupported D3DDECLTYPE ('{}') for usagae '{}' "
                    "encountered. "
                    "Rendering maybe buggy or SP may even crash.",
                    elem.Type, desc.semantic_name);
         } break;
         }
      } break;
      case D3DDECLUSAGE_BLENDINDICES:
      case D3DDECLUSAGE_COLOR: {
         switch (elem.Type) {
         case D3DDECLTYPE_D3DCOLOR: {
            desc.format = DXGI_FORMAT_B8G8R8A8_UNORM;
         } break;
         default: {
            is_valid = false;

            log_fmt(Log_level::warning,
                    "Unknown or unsupported D3DDECLTYPE ('{}') for usagae '{}' "
                    "encountered. "
                    "Rendering maybe buggy or SP may even crash.",
                    elem.Type, desc.semantic_name);
         } break;
         }
      } break;
      case D3DDECLUSAGE_NORMAL:
      case D3DDECLUSAGE_TANGENT:
      case D3DDECLUSAGE_BINORMAL: {
         switch (elem.Type) {
         case D3DDECLTYPE_D3DCOLOR: {
            desc.format = DXGI_FORMAT_B8G8R8A8_UNORM;
         } break;
         case D3DDECLTYPE_FLOAT3: {
            desc.format = DXGI_FORMAT_R32G32B32_FLOAT;
         } break;
         default: {
            is_valid = false;

            log_fmt(Log_level::warning,
                    "Unknown or unsupported D3DDECLTYPE ('{}') for usagae '{}' "
                    "encountered. "
                    "Rendering maybe buggy or SP may even crash.",
                    elem.Type, desc.semantic_name);
         } break;
         }
      } break;
      case D3DDECLUSAGE_TEXCOORD: {
         switch (elem.Type) {
         case D3DDECLTYPE_FLOAT2: {
            desc.format = DXGI_FORMAT_R32G32_FLOAT;
         } break;
         case D3DDECLTYPE_SHORT2: {
            desc.format = DXGI_FORMAT_R16G16_SINT;

            compressed_texcoords = true;
         } break;
         default: {
            is_valid = false;

            log_fmt(Log_level::warning,
                    "Unknown or unsupported D3DDECLTYPE ('{}') for usagae '{}' "
                    "encountered. "
                    "Rendering maybe buggy or SP may even crash.",
                    elem.Type, desc.semantic_name);
         } break;
         }
      } break;
      case D3DDECLUSAGE_POSITIONT: {
         desc.format =
            d3d_decl_type_to_dxgi_format(static_cast<D3DDECLTYPE>(elem.Type));
      } break;
      default: {
         is_valid = false;

         log_fmt(Log_level::warning,
                 "Unknown or unsupported D3DDECLUSAGE ('{}') encountered. "
                 "Rendering maybe buggy or SP may even crash.",
                 desc.semantic_name);
      } break;
      }

      if (is_valid) elements.push_back(desc);
   }

   return shader_patch.create_game_input_layout(elements, compressed_position,
                                                compressed_texcoords,
                                                has_vertex_weights);
}
}

Com_ptr<Vertex_declaration> Vertex_declaration::create(
   core::Shader_patch& shader_patch,
   const std::span<const D3DVERTEXELEMENT9> elements) noexcept
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
                                       const std::span<const D3DVERTEXELEMENT9> elements) noexcept
   : _input_layout{create_input_layout(shader_patch, elements)}
{
}
}
