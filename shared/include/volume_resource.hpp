#pragma once

#include "magic_number.hpp"
#include "ucfb_writer.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string_view>
#include <utility>
#include <vector>

#include <gsl/gsl>

namespace sp {

enum class Volume_resource_type : std::uint32_t {
   material,
   texture,
   fx_config,
   colorgrading_regions
};

extern const std::int32_t volume_resource_format;

struct Volume_resource_header {
   const Magic_number mn = "spvr"_mn;
   Volume_resource_type type;
   std::uint32_t payload_size;
   std::uint32_t _reserved{};
};

static_assert(sizeof(Volume_resource_header) == 16);

void write_volume_resource(ucfb::Writer& writer, const std::string_view name,
                           const Volume_resource_type type,
                           gsl::span<const std::byte> data);

void save_volume_resource(const std::filesystem::path& output_path,
                          const std::string_view name, const Volume_resource_type type,
                          gsl::span<const std::byte> data);

auto load_volume_resource(const std::filesystem::path& path)
   -> std::pair<Volume_resource_header, std::vector<std::byte>>;
}
