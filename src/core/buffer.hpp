#pragma once

#include "com_ptr.hpp"

#include <d3d11_4.h>

#include <absl/container/inlined_vector.h>

namespace sp::core {

struct Dynamic_buffer_instance {
   Com_ptr<ID3D11Buffer> buffer;
   UINT64 last_used;
};

struct Buffer {
   constexpr static std::size_t initial_dynamic_instances = 2;

   Com_ptr<ID3D11Buffer> buffer;

   absl::InlinedVector<Dynamic_buffer_instance, 3> dynamic_instances;
};

constexpr auto value = sizeof absl::InlinedVector<Dynamic_buffer_instance, 3>;

}
