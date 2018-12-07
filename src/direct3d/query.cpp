
#include "query.hpp"
#include "debug_trace.hpp"

#include <algorithm>
#include <cstring>
#include <limits>

namespace sp::d3d9 {

namespace {

template<typename Type>
class Basic_query final : public IDirect3DQuery9, private Type {
public:
   static Com_ptr<Basic_query> create(core::Shader_patch& shader_patch) noexcept
   {
      return Com_ptr{new Basic_query{shader_patch}};
   }

   Basic_query(const Basic_query&) = delete;
   Basic_query& operator=(const Basic_query&) = delete;

   Basic_query(Basic_query&&) = delete;
   Basic_query& operator=(Basic_query&&) = delete;

   HRESULT __stdcall QueryInterface(const IID& iid, void** object) noexcept override
   {
      Debug_trace::func(__FUNCSIG__);

      if (!object) return E_INVALIDARG;

      if (iid == IID_IUnknown) {
         *object = static_cast<IUnknown*>(this);
      }
      else if (iid == IID_IDirect3DQuery9) {
         *object = this;
      }
      else {
         *object = nullptr;

         return E_NOINTERFACE;
      }

      AddRef();

      return S_OK;
   }

   ULONG __stdcall AddRef() noexcept override
   {
      Debug_trace::func(__FUNCSIG__);

      return _ref_count += 1;
   }

   ULONG __stdcall Release() noexcept override
   {
      Debug_trace::func(__FUNCSIG__);

      const auto ref_count = _ref_count -= 1;

      if (ref_count == 0) delete this;

      return ref_count;
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetDevice(IDirect3DDevice9**) noexcept override final
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   D3DQUERYTYPE __stdcall GetType() noexcept override
   {
      Debug_trace::func(__FUNCSIG__);

      return Type::d3d9_type;
   }

   DWORD __stdcall GetDataSize() noexcept override
   {
      Debug_trace::func(__FUNCSIG__);

      switch (Type::d3d9_type) {
      case D3DQUERYTYPE_VCACHE:
         return sizeof(D3DDEVINFO_VCACHE);
      case D3DQUERYTYPE_VERTEXSTATS:
         return sizeof(D3DDEVINFO_D3DVERTEXSTATS);
      case D3DQUERYTYPE_EVENT:
         return sizeof(BOOL);
      case D3DQUERYTYPE_OCCLUSION:
         return sizeof(DWORD);
      case D3DQUERYTYPE_TIMESTAMP:
         return sizeof(UINT64);
      case D3DQUERYTYPE_TIMESTAMPDISJOINT:
         return sizeof(BOOL);
      case D3DQUERYTYPE_TIMESTAMPFREQ:
         return sizeof(UINT64);
      case D3DQUERYTYPE_PIPELINETIMINGS:
         return sizeof(D3DDEVINFO_D3D9PIPELINETIMINGS);
      case D3DQUERYTYPE_INTERFACETIMINGS:
         return sizeof(D3DDEVINFO_D3D9INTERFACETIMINGS);
      case D3DQUERYTYPE_VERTEXTIMINGS:
         return sizeof(D3DDEVINFO_D3D9STAGETIMINGS);
      case D3DQUERYTYPE_PIXELTIMINGS:
         return sizeof(D3DDEVINFO_D3D9STAGETIMINGS);
      case D3DQUERYTYPE_BANDWIDTHTIMINGS:
         return sizeof(D3DDEVINFO_D3D9BANDWIDTHTIMINGS);
      case D3DQUERYTYPE_CACHEUTILIZATION:
         return sizeof(D3DDEVINFO_D3D9CACHEUTILIZATION);
      default:
         log_and_terminate("Unexpected query type in " __FUNCSIG__);
      }
   }

   HRESULT __stdcall Issue(DWORD issue_flags) noexcept override
   {
      Debug_trace::func(__FUNCSIG__);

      if (issue_flags == D3DISSUE_BEGIN) {
         Type::begin();

         return S_OK;
      }

      if (issue_flags == D3DISSUE_END) {
         Type::end();

         return S_OK;
      }

      return D3DERR_INVALIDCALL;
   }

   HRESULT __stdcall GetData(void* data, DWORD size, DWORD flags) noexcept override
   {
      Debug_trace::func(__FUNCSIG__);

      if (!data) return D3DERR_INVALIDCALL;
      if (size != GetDataSize()) return D3DERR_INVALIDCALL;

      const bool flush = flags == D3DGETDATA_FLUSH;

      const auto result =
         Type::get_data(flush, gsl::make_span(static_cast<std::byte*>(data), size));

      switch (result) {
      case core::Query_result::success:
         return S_OK;
      case core::Query_result::notready:
         return S_FALSE;
      case core::Query_result::error:
      default:
         return D3DERR_INVALIDCALL;
      }
   }

private:
   Basic_query(core::Shader_patch& shader_patch) noexcept : Type{shader_patch}
   {
   }

   ~Basic_query() = default;

   ULONG _ref_count = 1;
};

class Event_query {
public:
   constexpr static auto d3d9_type = D3DQUERYTYPE_EVENT;

   Event_query(core::Shader_patch& shader_patch) noexcept
      : _shader_patch{shader_patch}
   {
   }

   void begin() noexcept {}

   void end() noexcept
   {
      _shader_patch.end_query(*_query);
   }

   auto get_data(const bool flush, gsl::span<std::byte> data) noexcept -> core::Query_result
   {
      Expects(data.size() == sizeof(BOOL));

      return _shader_patch.get_query_data(*_query, flush, data);
   }

private:
   core::Shader_patch& _shader_patch;
   const Com_ptr<ID3D11Query> _query{
      _shader_patch.create_query(CD3D11_QUERY_DESC{D3D11_QUERY_EVENT})};
};

class Occlusion_query {
public:
   constexpr static auto d3d9_type = D3DQUERYTYPE_OCCLUSION;

   Occlusion_query(core::Shader_patch& shader_patch) noexcept
      : _shader_patch{shader_patch}
   {
   }

   void begin() noexcept
   {
      _shader_patch.begin_query(*_query);
   }

   void end() noexcept
   {
      _shader_patch.end_query(*_query);
   }

   auto get_data(const bool flush, gsl::span<std::byte> data) noexcept -> core::Query_result
   {
      Expects(data.size() == sizeof(DWORD));

      UINT64 sample_count;

      const auto result =
         _shader_patch.get_query_data(*_query, flush,
                                      gsl::make_span(reinterpret_cast<std::byte*>(&sample_count),
                                                     sizeof(sample_count)));

      if (result != core::Query_result::success) return result;

      const DWORD dword_sample_count = static_cast<DWORD>(
         std::clamp(sample_count, UINT64{std::numeric_limits<DWORD>::min()},
                    UINT64{std::numeric_limits<DWORD>::max()}));

      std::memcpy(data.data(), &dword_sample_count, sizeof(DWORD));

      return result;
   }

private:
   core::Shader_patch& _shader_patch;
   const Com_ptr<ID3D11Query> _query{
      _shader_patch.create_query(CD3D11_QUERY_DESC{D3D11_QUERY_OCCLUSION})};
};

}

Com_ptr<IDirect3DQuery9> make_query(core::Shader_patch& shader_patch,
                                    const D3DQUERYTYPE type) noexcept
{
   Expects(type == D3DQUERYTYPE_EVENT || type == D3DQUERYTYPE_OCCLUSION);

   if (type == D3DQUERYTYPE_EVENT) {
      return Basic_query<Event_query>::create(shader_patch);
   }
   else if (type == D3DQUERYTYPE_OCCLUSION) {
      return Basic_query<Occlusion_query>::create(shader_patch);
   }

   std::terminate();
}

}
