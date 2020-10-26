
#include "color_grading_regions_io.hpp"
#include "ucfb_writer.hpp"
#include "volume_resource.hpp"

#include <limits>
#include <span>

#include <gsl/gsl>

namespace sp {

namespace {

enum class Colorgrading_version : std::uint32_t { v_1, current = v_1 };

}

void write_colorgrading_regions(const std::filesystem::path& save_path,
                                const Color_grading_regions& colorgrading_regions)
{
   Expects(colorgrading_regions.regions.size() <=
           std::numeric_limits<std::uint32_t>::max());
   Expects(colorgrading_regions.configs.size() <=
           std::numeric_limits<std::uint32_t>::max());

   std::ostringstream ostream;

   // write colorgrading regions chunk
   {
      ucfb::Writer writer{ostream, "clrg"_mn};

      writer.write(Colorgrading_version::current);

      // write region count
      writer.write<std::uint32_t>(
         static_cast<std::uint32_t>(colorgrading_regions.regions.size()));

      for (const auto& region : colorgrading_regions.regions) {
         writer.write(region.name, region.config_name, region.shape, region.rotation,
                      region.position, region.size, region.fade_length);
      }

      // write config count
      writer.write<std::uint32_t>(
         static_cast<std::uint32_t>(colorgrading_regions.configs.size()));

      for (const auto& config : colorgrading_regions.configs) {
         writer.write(config.first);

         const auto yaml = YAML::Dump(config.second);

         if (yaml.size() > std::numeric_limits<std::uint32_t>::max()) {
            throw std::runtime_error{"config too large!"};
         }

         writer.write<std::uint32_t>(static_cast<std::uint32_t>(yaml.size()));
         writer.write(std::span{yaml});
      }
   }

   const auto regions_data = ostream.str();
   const auto regions_span = std::as_bytes(std::span{regions_data});

   save_volume_resource(save_path, save_path.stem().string(),
                        Volume_resource_type::colorgrading_regions, regions_span);
}

auto load_colorgrading_regions(ucfb::Reader_strict<"clrg"_mn> reader) -> Color_grading_regions
{
   const auto version = reader.read<Colorgrading_version>();

   if (version != Colorgrading_version::v_1) {
      throw std::runtime_error{"unexpected version for colorgrading regions!"};
   }

   Color_grading_regions colorgrading;

   const auto region_count = reader.read<std::uint32_t>();

   colorgrading.regions.resize(region_count);

   for (auto& regn : colorgrading.regions) {
      regn.name = reader.read_string();
      regn.config_name = reader.read_string();

      std::tie(regn.shape, regn.rotation, regn.position, regn.size, regn.fade_length) =
         reader.read_multi<Color_grading_region_shape, glm::quat, glm::vec3, glm::vec3, float>();
   }

   const auto config_count = reader.read<std::uint32_t>();

   for (auto i = 0; i < config_count; ++i) {
      const auto name = reader.read_string();

      const auto config_size = reader.read<std::uint32_t>();
      const auto config_data = reader.read_array<char>(config_size);

      colorgrading.configs.emplace(name, YAML::Load(std::string{config_data.data(),
                                                                config_size}));
   }

   return colorgrading;
}

}
