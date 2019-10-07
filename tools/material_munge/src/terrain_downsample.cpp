
#include "terrain_downsample.hpp"
#include "utility.hpp"

#include <cmath>
#include <limits>

#include <gsl/gsl>

namespace sp {

namespace {

auto index(const int length, const int x, const int y)
{
   const auto x_clamped = std::clamp(x, 0, length - 1);
   const auto y_clamped = std::clamp(y, 0, length - 1);

   return x_clamped + (y_clamped * length);
}

template<typename Type>
auto array_type_len(const Type& type) -> decltype(type.length())
{
   return type.length();
}

template<typename Type>
auto array_type_len(const Type& type) -> decltype(type.size())
{
   return type.size();
}

template<typename Type>
auto sample(const std::unique_ptr<Type[]>& data, const int length,
            const int x_offs, const int y_offs, const int footprint) -> Type
{
   const auto inv_total_weight = 1.0f / (footprint * footprint);
   Type total{};

   for (auto y = y_offs; y < (y_offs + footprint); ++y) {
      for (auto x = x_offs; x < (x_offs + footprint); ++x) {
         const auto& v = data[index(length, x, y)];

         for (auto i = 0; i < array_type_len(v); ++i) {
            total[i] += v[i];
         }
      }
   }

   for (auto i = 0; i < array_type_len(total); ++i) {
      total[i] *= inv_total_weight;
   }

   return total;
}

void downsample_positions(const Terrain_map& input, const Terrain_map& output,
                          const int footprint) noexcept
{
   const auto x_begin = input.position[index(input.length, 0, 0)].x;
   const auto x_end = input.position[index(input.length, input.length - 1, 0)].x;
   const auto z_begin = input.position[index(input.length, 0, 0)].z;
   const auto z_end = input.position[index(input.length, 0, input.length - 1)].z;

   for (auto y = 0; y < output.length; ++y) {
      for (auto x = 0; x < output.length; ++x) {
         const float x_pos = glm::mix(x_begin, x_end, x / (output.length - 1.0f));

         const float z_pos = glm::mix(z_begin, z_end, y / (output.length - 1.0f));

         output.position[index(output.length, x, y)] = {x_pos,
                                                        sample(input.position,
                                                               input.length, x * footprint,
                                                               y * footprint, footprint)
                                                           .y,
                                                        z_pos};
      }
   }
}

void downsample_color(const Terrain_map& input, const Terrain_map& output,
                      const int footprint)
{
   for (auto y = 0; y < output.length; ++y) {
      for (auto x = 0; x < output.length; ++x) {
         output.color[index(output.length, x, y)] =
            sample(input.color, input.length, x * footprint, y * footprint, footprint);
      }
   }
}

void downsample_diffuse_lighting(const Terrain_map& input,
                                 const Terrain_map& output, const int footprint)
{
   for (auto y = 0; y < output.length; ++y) {
      for (auto x = 0; x < output.length; ++x) {
         output.diffuse_lighting[index(output.length, x, y)] =
            sample(input.diffuse_lighting, input.length, x * footprint,
                   y * footprint, footprint);
      }
   }
}

void downsample_texture_weights(const Terrain_map& input,
                                const Terrain_map& output, const int footprint)
{
   for (auto y = 0; y < output.length; ++y) {
      for (auto x = 0; x < output.length; ++x) {
         output.texture_weights[index(output.length, x, y)] =
            sample(input.texture_weights, input.length, x * footprint,
                   y * footprint, footprint);
      }
   }
}
}

auto terrain_downsample(const Terrain_map& input, const std::uint16_t new_length)
   -> Terrain_map
{
   Expects(new_length > 1);

   if (new_length >= input.length) return Terrain_map{input};

   Terrain_map output{new_length};

   output.texture_names = input.texture_names;
   output.texture_transforms = input.texture_transforms;
   output.cuts = input.cuts;

   const auto footprint = static_cast<int>(std::ceil(
      static_cast<double>(input.length) / static_cast<double>(new_length)));

   downsample_positions(input, output, footprint);
   downsample_color(input, output, footprint);
   downsample_diffuse_lighting(input, output, footprint);
   downsample_texture_weights(input, output, footprint);

   return output;
}
}
