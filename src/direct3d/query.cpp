
#include "query.hpp"
#include "debug_trace.hpp"

#include <algorithm>
#include <cstring>
#include <limits>
#include <span>

namespace sp::d3d9 {

namespace {

template<D3DQUERYTYPE query_type>
class Basic_query final : public IDirect3DQuery9 {
public:
   static_assert(query_type == D3DQUERYTYPE_EVENT ||
                 query_type == D3DQUERYTYPE_OCCLUSION);

   static Com_ptr<Basic_query> create() noexcept
   {
      return Com_ptr{new Basic_query{}};
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

      return query_type;
   }

   DWORD __stdcall GetDataSize() noexcept override
   {
      Debug_trace::func(__FUNCSIG__);

      if constexpr (query_type == D3DQUERYTYPE_EVENT) {
         return sizeof(BOOL);
      }
      else if constexpr (query_type == D3DQUERYTYPE_OCCLUSION) {
         return sizeof(DWORD);
      }
   }

   HRESULT __stdcall Issue([[maybe_unused]] DWORD issue_flags) noexcept override
   {
      Debug_trace::func(__FUNCSIG__);

      return S_OK;
   }

   HRESULT __stdcall GetData(void* data, DWORD size,
                             [[maybe_unused]] DWORD flags) noexcept override
   {
      Debug_trace::func(__FUNCSIG__);

      if (!data) return D3DERR_INVALIDCALL;
      if (size != GetDataSize()) return D3DERR_INVALIDCALL;

      if constexpr (query_type == D3DQUERYTYPE_EVENT) {
         // The game can spin on event queries, waiting for previous GPU work to
         // complete before continuing. Presumably there was a good reason for
         // this on some HW configuration when the game came out.
         //
         // It also however could cause the GPU and CPU to become synchronous,
         // as a result we simply always report the event as having completed.

         constexpr BOOL truth_bool = true;

         std::memcpy(data, &truth_bool, sizeof(BOOL));

         return S_OK;
      }
      else if constexpr (query_type == D3DQUERYTYPE_OCCLUSION) {
         // The game (at least it seems to) attempts to use occlusion queries from the Z-prepass
         // to cull meshes from the main pass. It however does this on the CPU timeline.
         // Meaning that in practise these days the queries are never ready by the time the game uses them,
         // this isn't harmful to performance but it does mean there is no point in us
         // implementing occlusion queries for the game.

         constexpr DWORD zero_dword = 0;

         std::memcpy(data, &zero_dword, sizeof(DWORD));

         return S_FALSE;
      }
   }

private:
   Basic_query() noexcept = default;

   ~Basic_query() = default;

   ULONG _ref_count = 1;
};

}

Com_ptr<IDirect3DQuery9> make_query(const D3DQUERYTYPE type) noexcept
{
   if (type == D3DQUERYTYPE_EVENT) {
      return Basic_query<D3DQUERYTYPE_EVENT>::create();
   }
   else if (type == D3DQUERYTYPE_OCCLUSION) {
      return Basic_query<D3DQUERYTYPE_OCCLUSION>::create();
   }

   log_and_terminate("Unexpected query type.");
}

}
