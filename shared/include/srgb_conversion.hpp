#pragma once

#include <cmath>

#include <glm/glm.hpp>

namespace sp {

template<typename Float>
inline auto decompress_srgb(const Float v) -> Float
{
   return (v < Float{0.04045})
             ? v / Float{12.92}
             : std::pow(std::abs((v + Float{0.055})) / Float{1.055}, Float{2.4});
}

inline auto decompress_srgb(const glm::vec3 color) -> glm::vec3
{
   return {decompress_srgb(color.r), decompress_srgb(color.g),
           decompress_srgb(color.b)};
}

inline auto decompress_srgb(const glm::vec4 color) -> glm::vec4
{
   return {decompress_srgb(color.r), decompress_srgb(color.g),
           decompress_srgb(color.b), color.a};
}

template<typename Float>
inline auto compress_srgb(const Float v)
{
   return (v < Float{0.0031308})
             ? v * Float{12.92}
             : Float{1.055} * std::pow(std::abs(v), Float{1.0} / Float{2.4}) -
                  Float{0.055};
}

inline auto compress_srgb(const glm::vec3 color) -> glm::vec3
{
   return {compress_srgb(color.r), compress_srgb(color.g), compress_srgb(color.b)};
}

inline auto compress_srgb(const glm::vec4 color) -> glm::vec4
{
   return {compress_srgb(color.r), compress_srgb(color.g),
           compress_srgb(color.b), color.a};
}

}
