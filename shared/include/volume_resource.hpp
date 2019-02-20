#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include <gsl/gsl>

namespace sp {

enum class Volume_resource_type : std::uint16_t {
   material,
   texture,
   fx_config
};

extern const std::uint32_t volume_resource_format;

void save_volume_resource(const std::string& output_path, std::string_view name,
                          Volume_resource_type type, gsl::span<const std::byte> data);

}
