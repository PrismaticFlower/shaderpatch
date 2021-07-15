#pragma once

#include "com_ptr.hpp"
#include "small_mutex.hpp"

#include <d3d11_4.h>

#include <absl/container/inlined_vector.h>

namespace sp::core {

struct Dynamic_buffer_instance {
   Com_ptr<ID3D11Buffer> buffer;
   UINT64 last_used = 0;
};

struct Buffer {
   constexpr static std::size_t initial_dynamic_instances = 2;

   Com_ptr<ID3D11Buffer> buffer;

   small_mutex dynamic_instances_mutex;
   absl::InlinedVector<Dynamic_buffer_instance, 3> dynamic_instances;

   std::size_t cpu_active_dynamic_instance = 0;
};

}
