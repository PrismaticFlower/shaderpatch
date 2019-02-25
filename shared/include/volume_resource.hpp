#pragma once

#include "magic_number.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include <gsl/gsl>

namespace sp {

enum class Volume_resource_type : std::uint32_t {
   material,
   texture,
   fx_config
};

extern const std::int32_t volume_resource_format;

struct Volume_resource_header {
   const Magic_number mn = "spvr"_mn;
   Volume_resource_type type;
   std::uint32_t payload_size;
   std::uint32_t _reserved{};
};

static_assert(sizeof(Volume_resource_header) == 16);

void save_volume_resource(const std::string& output_path, std::string_view name,
                          Volume_resource_type type, gsl::span<const std::byte> data);

}
