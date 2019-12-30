
#pragma warning(disable : 4701) // For GLM - potentially uninitialized local variable used

#include "image_span.hpp"
#include "srgb_conversion.hpp"

#include <glm/gtc/packing.hpp>
#include <gsl/gsl>

namespace sp {

namespace {

template<typename Pointer>
inline auto texel_address(const glm::ivec2 index, const std::size_t row_pitch,
                          const std::size_t texel_size, Pointer* const data) noexcept
   -> Pointer*
{
   static_assert(std::is_same_v<std::remove_cv_t<Pointer>, std::byte>);

   auto* const row = data + (row_pitch * index.y);
   auto* const texel = row + (texel_size * index.x);

   return texel;
}

auto load_r32g32b32a32_float(const glm::ivec2 index, const std::size_t row_pitch,
                             const std::byte* const data) noexcept -> glm::vec4
{
   constexpr auto texel_size = sizeof(glm::vec4);

   glm::vec4 value;

   std::memcpy(&value, texel_address(index, row_pitch, texel_size, data), texel_size);

   return value;
}

void store_r32g32b32a32_float(const glm::vec4 value, const glm::ivec2 index,
                              const std::size_t row_pitch, std::byte* const data) noexcept
{
   constexpr auto texel_size = sizeof(glm::vec4);

   std::memcpy(texel_address(index, row_pitch, texel_size, data), &value, texel_size);
}

auto load_r32g32b32_float(const glm::ivec2 index, const std::size_t row_pitch,
                          const std::byte* const data) noexcept -> glm::vec4
{
   constexpr auto texel_size = sizeof(glm::vec3);

   glm::vec3 value;

   std::memcpy(&value, texel_address(index, row_pitch, texel_size, data), texel_size);

   return {value, 1.0f};
}

void store_r32g32b32_float(const glm::vec4 value, const glm::ivec2 index,
                           const std::size_t row_pitch, std::byte* const data) noexcept
{
   constexpr auto texel_size = sizeof(glm::vec3);

   std::memcpy(texel_address(index, row_pitch, texel_size, data), &value, texel_size);
}

auto load_r32g32_float(const glm::ivec2 index, const std::size_t row_pitch,
                       const std::byte* const data) noexcept -> glm::vec4
{
   constexpr auto texel_size = sizeof(glm::vec2);

   glm::vec2 value;

   std::memcpy(&value, texel_address(index, row_pitch, texel_size, data), texel_size);

   return {value, 0.0f, 1.0f};
}

void store_r32g32_float(const glm::vec4 value, const glm::ivec2 index,
                        const std::size_t row_pitch, std::byte* const data) noexcept
{
   constexpr auto texel_size = sizeof(glm::vec2);

   std::memcpy(texel_address(index, row_pitch, texel_size, data), &value, texel_size);
}

auto load_r32_float(const glm::ivec2 index, const std::size_t row_pitch,
                    const std::byte* const data) noexcept -> glm::vec4
{
   constexpr auto texel_size = sizeof(float);

   float value;

   std::memcpy(&value, texel_address(index, row_pitch, texel_size, data), texel_size);

   return {value, 0.0f, 0.0f, 1.0f};
}

void store_r32_float(const glm::vec4 value, const glm::ivec2 index,
                     const std::size_t row_pitch, std::byte* const data) noexcept
{
   constexpr auto texel_size = sizeof(float);

   std::memcpy(texel_address(index, row_pitch, texel_size, data), &value, texel_size);
}

auto load_r16g16b16a16_float(const glm::ivec2 index, const std::size_t row_pitch,
                             const std::byte* const data) noexcept -> glm::vec4
{
   constexpr auto texel_size = sizeof(glm::uint64);

   glm::uint64 value;

   std::memcpy(&value, texel_address(index, row_pitch, texel_size, data), texel_size);

   return glm::unpackHalf4x16(value);
}

void store_r16g16b16a16_float(const glm::vec4 value, const glm::ivec2 index,
                              const std::size_t row_pitch, std::byte* const data) noexcept
{
   constexpr auto texel_size = sizeof(glm::uint64);

   const auto packed = glm::packHalf4x16(value);

   std::memcpy(texel_address(index, row_pitch, texel_size, data), &packed, texel_size);
}

auto load_r16g16b16a16_unorm(const glm::ivec2 index, const std::size_t row_pitch,
                             const std::byte* const data) noexcept -> glm::vec4
{
   constexpr auto texel_size = sizeof(glm::uint64);

   glm::uint64 value;

   std::memcpy(&value, texel_address(index, row_pitch, texel_size, data), texel_size);

   return glm::unpackUnorm4x16(value);
}

void store_r16g16b16a16_unorm(const glm::vec4 value, const glm::ivec2 index,
                              const std::size_t row_pitch, std::byte* const data) noexcept
{
   constexpr auto texel_size = sizeof(glm::uint64);

   const auto packed = glm::packUnorm4x16(value);

   std::memcpy(texel_address(index, row_pitch, texel_size, data), &packed, texel_size);
}

auto load_r16g16b16a16_snorm(const glm::ivec2 index, const std::size_t row_pitch,
                             const std::byte* const data) noexcept -> glm::vec4
{
   constexpr auto texel_size = sizeof(glm::uint64);

   glm::uint64 value;

   std::memcpy(&value, texel_address(index, row_pitch, texel_size, data), texel_size);

   return glm::unpackSnorm4x16(value);
}

void store_r16g16b16a16_snorm(const glm::vec4 value, const glm::ivec2 index,
                              const std::size_t row_pitch, std::byte* const data) noexcept
{
   constexpr auto texel_size = sizeof(glm::uint64);

   const auto packed = glm::packSnorm4x16(value);

   std::memcpy(texel_address(index, row_pitch, texel_size, data), &packed, texel_size);
}

auto load_r16g16_float(const glm::ivec2 index, const std::size_t row_pitch,
                       const std::byte* const data) noexcept -> glm::vec4
{
   constexpr auto texel_size = sizeof(glm::uint32);

   glm::uint32 value;

   std::memcpy(&value, texel_address(index, row_pitch, texel_size, data), texel_size);

   return {glm::unpackHalf2x16(value), 0.0f, 1.0f};
}

void store_r16g16_float(const glm::vec4 value, const glm::ivec2 index,
                        const std::size_t row_pitch, std::byte* const data) noexcept
{
   constexpr auto texel_size = sizeof(glm::uint32);

   const auto packed = glm::packHalf2x16(value);

   std::memcpy(texel_address(index, row_pitch, texel_size, data), &packed, texel_size);
}

auto load_r16g16_unorm(const glm::ivec2 index, const std::size_t row_pitch,
                       const std::byte* const data) noexcept -> glm::vec4
{
   constexpr auto texel_size = sizeof(glm::uint32);

   glm::uint32 value;

   std::memcpy(&value, texel_address(index, row_pitch, texel_size, data), texel_size);

   return {glm::unpackUnorm2x16(value), 0.0f, 1.0f};
}

void store_r16g16_unorm(const glm::vec4 value, const glm::ivec2 index,
                        const std::size_t row_pitch, std::byte* const data) noexcept
{
   constexpr auto texel_size = sizeof(glm::uint32);

   const auto packed = glm::packUnorm2x16(value);

   std::memcpy(texel_address(index, row_pitch, texel_size, data), &packed, texel_size);
}

auto load_r16g16_snorm(const glm::ivec2 index, const std::size_t row_pitch,
                       const std::byte* const data) noexcept -> glm::vec4
{
   constexpr auto texel_size = sizeof(glm::uint32);

   glm::uint32 value;

   std::memcpy(&value, texel_address(index, row_pitch, texel_size, data), texel_size);

   return {glm::unpackSnorm2x16(value), 0.0f, 1.0f};
}

void store_r16g16_snorm(const glm::vec4 value, const glm::ivec2 index,
                        const std::size_t row_pitch, std::byte* const data) noexcept
{
   constexpr auto texel_size = sizeof(glm::uint32);

   const auto packed = glm::packSnorm2x16(value);

   std::memcpy(texel_address(index, row_pitch, texel_size, data), &packed, texel_size);
}

auto load_r16_float(const glm::ivec2 index, const std::size_t row_pitch,
                    const std::byte* const data) noexcept -> glm::vec4
{
   constexpr auto texel_size = sizeof(glm::uint16);

   glm::uint16 value;

   std::memcpy(&value, texel_address(index, row_pitch, texel_size, data), texel_size);

   return {glm::unpackHalf1x16(value), 0.0f, 0.0f, 1.0f};
}

void store_r16_float(const glm::vec4 value, const glm::ivec2 index,
                     const std::size_t row_pitch, std::byte* const data) noexcept
{
   constexpr auto texel_size = sizeof(glm::uint16);

   const auto packed = glm::packHalf1x16(value.x);

   std::memcpy(texel_address(index, row_pitch, texel_size, data), &packed, texel_size);
}

auto load_r16_unorm(const glm::ivec2 index, const std::size_t row_pitch,
                    const std::byte* const data) noexcept -> glm::vec4
{
   constexpr auto texel_size = sizeof(glm::uint16);

   glm::uint16 value;

   std::memcpy(&value, texel_address(index, row_pitch, texel_size, data), texel_size);

   return {glm::unpackUnorm1x16(value), 0.0f, 0.0f, 1.0f};
}

void store_r16_unorm(const glm::vec4 value, const glm::ivec2 index,
                     const std::size_t row_pitch, std::byte* const data) noexcept
{
   constexpr auto texel_size = sizeof(glm::uint16);

   const auto packed = glm::packUnorm1x16(value.x);

   std::memcpy(texel_address(index, row_pitch, texel_size, data), &packed, texel_size);
}

auto load_r16_snorm(const glm::ivec2 index, const std::size_t row_pitch,
                    const std::byte* const data) noexcept -> glm::vec4
{
   constexpr auto texel_size = sizeof(glm::uint16);

   glm::uint16 value;

   std::memcpy(&value, texel_address(index, row_pitch, texel_size, data), texel_size);

   return {glm::unpackSnorm1x16(value), 0.0f, 0.0f, 1.0f};
}

void store_r16_snorm(const glm::vec4 value, const glm::ivec2 index,
                     const std::size_t row_pitch, std::byte* const data) noexcept
{
   constexpr auto texel_size = sizeof(glm::uint16);

   const auto packed = glm::packSnorm1x16(value.x);

   std::memcpy(texel_address(index, row_pitch, texel_size, data), &packed, texel_size);
}

auto load_r11g11b10_float(const glm::ivec2 index, const std::size_t row_pitch,
                          const std::byte* const data) noexcept -> glm::vec4
{
   constexpr auto texel_size = sizeof(glm::uint32);

   glm::uint32 value;

   std::memcpy(&value, texel_address(index, row_pitch, texel_size, data), texel_size);

   return {glm::unpackF2x11_1x10(value), 1.0f};
}

void store_r11g11b10_float(const glm::vec4 value, const glm::ivec2 index,
                           const std::size_t row_pitch, std::byte* const data) noexcept
{
   constexpr auto texel_size = sizeof(glm::uint32);

   const auto packed = glm::packF2x11_1x10(value);

   std::memcpy(texel_address(index, row_pitch, texel_size, data), &packed, texel_size);
}

auto load_r10g10b10a2_unorm(const glm::ivec2 index, const std::size_t row_pitch,
                            const std::byte* const data) noexcept -> glm::vec4
{
   constexpr auto texel_size = sizeof(glm::uint32);

   glm::uint32 value;

   std::memcpy(&value, texel_address(index, row_pitch, texel_size, data), texel_size);

   return glm::unpackUnorm3x10_1x2(value);
}

void store_r10g10b10a2_unorm(const glm::vec4 value, const glm::ivec2 index,
                             const std::size_t row_pitch, std::byte* const data) noexcept
{
   constexpr auto texel_size = sizeof(glm::uint32);

   const auto packed = glm::packUnorm3x10_1x2(value);

   std::memcpy(texel_address(index, row_pitch, texel_size, data), &packed, texel_size);
}

auto load_r8g8b8a8_unorm(const glm::ivec2 index, const std::size_t row_pitch,
                         const std::byte* const data) noexcept -> glm::vec4
{
   constexpr auto texel_size = sizeof(glm::uint32);

   glm::uint32 value;

   std::memcpy(&value, texel_address(index, row_pitch, texel_size, data), texel_size);

   return glm::unpackUnorm4x8(value);
}

void store_r8g8b8a8_unorm(const glm::vec4 value, const glm::ivec2 index,
                          const std::size_t row_pitch, std::byte* const data) noexcept
{
   constexpr auto texel_size = sizeof(glm::uint32);

   const auto packed = glm::packUnorm4x8(value);

   std::memcpy(texel_address(index, row_pitch, texel_size, data), &packed, texel_size);
}

auto load_r8g8b8a8_unorm_srgb(const glm::ivec2 index, const std::size_t row_pitch,
                              const std::byte* const data) noexcept -> glm::vec4
{
   constexpr auto texel_size = sizeof(glm::uint32);

   glm::uint32 value;

   std::memcpy(&value, texel_address(index, row_pitch, texel_size, data), texel_size);

   return decompress_srgb(glm::unpackUnorm4x8(value));
}

void store_r8g8b8a8_unorm_srgb(const glm::vec4 value, const glm::ivec2 index,
                               const std::size_t row_pitch, std::byte* const data) noexcept
{
   constexpr auto texel_size = sizeof(glm::uint32);

   const auto packed = glm::packUnorm4x8(compress_srgb(value));

   std::memcpy(texel_address(index, row_pitch, texel_size, data), &packed, texel_size);
}

auto load_r8g8b8a8_snorm(const glm::ivec2 index, const std::size_t row_pitch,
                         const std::byte* const data) noexcept -> glm::vec4
{
   constexpr auto texel_size = sizeof(glm::uint32);

   glm::uint32 value;

   std::memcpy(&value, texel_address(index, row_pitch, texel_size, data), texel_size);

   return glm::unpackSnorm4x8(value);
}

void store_r8g8b8a8_snorm(const glm::vec4 value, const glm::ivec2 index,
                          const std::size_t row_pitch, std::byte* const data) noexcept
{
   constexpr auto texel_size = sizeof(glm::uint32);

   const auto packed = glm::packSnorm4x8(value);

   std::memcpy(texel_address(index, row_pitch, texel_size, data), &packed, texel_size);
}

auto load_r8g8_unorm(const glm::ivec2 index, const std::size_t row_pitch,
                     const std::byte* const data) noexcept -> glm::vec4
{
   constexpr auto texel_size = sizeof(glm::uint16);

   glm::uint16 value;

   std::memcpy(&value, texel_address(index, row_pitch, texel_size, data), texel_size);

   return {glm::unpackUnorm2x8(value), 0.0f, 1.0f};
}

void store_r8g8_unorm(const glm::vec4 value, const glm::ivec2 index,
                      const std::size_t row_pitch, std::byte* const data) noexcept
{
   constexpr auto texel_size = sizeof(glm::uint16);

   const auto packed = glm::packUnorm2x8(value);

   std::memcpy(texel_address(index, row_pitch, texel_size, data), &packed, texel_size);
}

auto load_r8g8_snorm(const glm::ivec2 index, const std::size_t row_pitch,
                     const std::byte* const data) noexcept -> glm::vec4
{
   constexpr auto texel_size = sizeof(glm::uint16);

   glm::uint16 value;

   std::memcpy(&value, texel_address(index, row_pitch, texel_size, data), texel_size);

   return {glm::unpackSnorm2x8(value), 0.0f, 1.0f};
}

void store_r8g8_snorm(const glm::vec4 value, const glm::ivec2 index,
                      const std::size_t row_pitch, std::byte* const data) noexcept
{
   constexpr auto texel_size = sizeof(glm::uint16);

   const auto packed = glm::packSnorm2x8(value);

   std::memcpy(texel_address(index, row_pitch, texel_size, data), &packed, texel_size);
}

auto load_r8_unorm(const glm::ivec2 index, const std::size_t row_pitch,
                   const std::byte* const data) noexcept -> glm::vec4
{
   constexpr auto texel_size = sizeof(glm::uint8);

   glm::uint8 value;

   std::memcpy(&value, texel_address(index, row_pitch, texel_size, data), texel_size);

   return {glm::unpackUnorm1x8(value), 0.0f, 0.0f, 1.0f};
}

void store_r8_unorm(const glm::vec4 value, const glm::ivec2 index,
                    const std::size_t row_pitch, std::byte* const data) noexcept
{
   constexpr auto texel_size = sizeof(glm::uint8);

   const auto packed = glm::packUnorm1x8(value.x);

   std::memcpy(texel_address(index, row_pitch, texel_size, data), &packed, texel_size);
}

auto load_r8_snorm(const glm::ivec2 index, const std::size_t row_pitch,
                   const std::byte* const data) noexcept -> glm::vec4
{
   constexpr auto texel_size = sizeof(glm::uint8);

   glm::uint8 value;

   std::memcpy(&value, texel_address(index, row_pitch, texel_size, data), texel_size);

   return {glm::unpackSnorm1x8(value), 0.0f, 0.0f, 1.0f};
}

void store_r8_snorm(const glm::vec4 value, const glm::ivec2 index,
                    const std::size_t row_pitch, std::byte* const data) noexcept
{
   constexpr auto texel_size = sizeof(glm::uint8);

   const auto packed = glm::packSnorm1x8(value.x);

   std::memcpy(texel_address(index, row_pitch, texel_size, data), &packed, texel_size);
}

auto load_a8_unorm(const glm::ivec2 index, const std::size_t row_pitch,
                   const std::byte* const data) noexcept -> glm::vec4
{
   return load_r8_unorm(index, row_pitch, data).gggr;
}

void store_a8_unorm(const glm::vec4 value, const glm::ivec2 index,
                    const std::size_t row_pitch, std::byte* const data) noexcept
{
   store_r8_unorm(value.aaaa, index, row_pitch, data);
}

auto load_r9g9b9e5_float(const glm::ivec2 index, const std::size_t row_pitch,
                         const std::byte* const data) noexcept -> glm::vec4
{
   constexpr auto texel_size = sizeof(glm::uint32);

   glm::uint32 value;

   std::memcpy(&value, texel_address(index, row_pitch, texel_size, data), texel_size);

   return {glm::unpackF3x9_E1x5(value), 1.0f};
}

void store_r9g9b9e5_float(const glm::vec4 value, const glm::ivec2 index,
                          const std::size_t row_pitch, std::byte* const data) noexcept
{
   constexpr auto texel_size = sizeof(glm::uint32);

   const auto packed = glm::packF3x9_E1x5(value);

   std::memcpy(texel_address(index, row_pitch, texel_size, data), &packed, texel_size);
}

auto load_b5g6r5_unorm(const glm::ivec2 index, const std::size_t row_pitch,
                       const std::byte* const data) noexcept -> glm::vec4
{
   constexpr auto texel_size = sizeof(glm::uint16);

   glm::uint16 value;

   std::memcpy(&value, texel_address(index, row_pitch, texel_size, data), texel_size);

   return {glm::unpackUnorm1x5_1x6_1x5(value).bgr, 1.0f};
}

void store_b5g6r5_unorm(const glm::vec4 value, const glm::ivec2 index,
                        const std::size_t row_pitch, std::byte* const data) noexcept
{
   constexpr auto texel_size = sizeof(glm::uint32);

   const auto packed = glm::packUnorm1x5_1x6_1x5(value.bgr);

   std::memcpy(texel_address(index, row_pitch, texel_size, data), &packed, texel_size);
}

auto load_b5g5r5a1_unorm(const glm::ivec2 index, const std::size_t row_pitch,
                         const std::byte* const data) noexcept -> glm::vec4
{
   constexpr auto texel_size = sizeof(glm::uint16);

   glm::uint16 value;

   std::memcpy(&value, texel_address(index, row_pitch, texel_size, data), texel_size);

   return glm::unpackUnorm3x5_1x1(value).bgra;
}

void store_b5g5r5a1_unorm(const glm::vec4 value, const glm::ivec2 index,
                          const std::size_t row_pitch, std::byte* const data) noexcept
{
   constexpr auto texel_size = sizeof(glm::uint32);

   const auto packed = glm::packUnorm3x5_1x1(value.bgra);

   std::memcpy(texel_address(index, row_pitch, texel_size, data), &packed, texel_size);
}

auto load_b8g8r8a8_unorm(const glm::ivec2 index, const std::size_t row_pitch,
                         const std::byte* const data) noexcept -> glm::vec4
{
   return load_r8g8b8a8_unorm(index, row_pitch, data).bgra;
}

void store_b8g8r8a8_unorm(const glm::vec4 value, const glm::ivec2 index,
                          const std::size_t row_pitch, std::byte* const data) noexcept
{
   store_r8g8b8a8_unorm(value.bgra, index, row_pitch, data);
}

auto load_b8g8r8a8_unorm_srgb(const glm::ivec2 index, const std::size_t row_pitch,
                              const std::byte* const data) noexcept -> glm::vec4
{
   return load_r8g8b8a8_unorm_srgb(index, row_pitch, data).bgra;
}

void store_b8g8r8a8_unorm_srgb(const glm::vec4 value, const glm::ivec2 index,
                               const std::size_t row_pitch, std::byte* const data) noexcept
{
   store_r8g8b8a8_unorm_srgb(value.bgra, index, row_pitch, data);
}

auto load_b8g8r8x8_unorm(const glm::ivec2 index, const std::size_t row_pitch,
                         const std::byte* const data) noexcept -> glm::vec4
{
   return {load_b8g8r8a8_unorm(index, row_pitch, data).rgb, 1.0f};
}

void store_b8g8r8x8_unorm(const glm::vec4 value, const glm::ivec2 index,
                          const std::size_t row_pitch, std::byte* const data) noexcept
{
   store_b8g8r8a8_unorm({value.rgb, 1.0f}, index, row_pitch, data);
}

auto load_b8g8r8x8_unorm_srgb(const glm::ivec2 index, const std::size_t row_pitch,
                              const std::byte* const data) noexcept -> glm::vec4
{
   return {load_b8g8r8a8_unorm_srgb(index, row_pitch, data).rgb, 1.0f};
}

void store_b8g8r8x8_unorm_srgb(const glm::vec4 value, const glm::ivec2 index,
                               const std::size_t row_pitch, std::byte* const data) noexcept
{
   store_b8g8r8a8_unorm_srgb({value.rgb, 1.0f}, index, row_pitch, data);
}

auto load_b4g4r4a4_unorm(const glm::ivec2 index, const std::size_t row_pitch,
                         const std::byte* const data) noexcept -> glm::vec4
{
   constexpr auto texel_size = sizeof(glm::uint16);

   glm::uint16 value;

   std::memcpy(&value, texel_address(index, row_pitch, texel_size, data), texel_size);

   return glm::unpackUnorm4x4(value).bgra;
}

void store_b4g4r4a4_unorm(const glm::vec4 value, const glm::ivec2 index,
                          const std::size_t row_pitch, std::byte* const data) noexcept
{
   constexpr auto texel_size = sizeof(glm::uint32);

   const auto packed = glm::packUnorm4x4(value.bgra);

   std::memcpy(texel_address(index, row_pitch, texel_size, data), &packed, texel_size);
}

auto get_load_function(const DXGI_FORMAT format) noexcept
{
   switch (format) {
   case DXGI_FORMAT_R32G32B32A32_FLOAT:
      return &load_r32g32b32a32_float;
   case DXGI_FORMAT_R32G32B32_FLOAT:
      return &load_r32g32b32_float;
   case DXGI_FORMAT_R32G32_FLOAT:
      return &load_r32g32_float;
   case DXGI_FORMAT_R32_FLOAT:
      return &load_r32_float;
   case DXGI_FORMAT_R16G16B16A16_FLOAT:
      return &load_r16g16b16a16_float;
   case DXGI_FORMAT_R16G16B16A16_UNORM:
      return &load_r16g16b16a16_unorm;
   case DXGI_FORMAT_R16G16B16A16_SNORM:
      return &load_r16g16b16a16_snorm;
   case DXGI_FORMAT_R16G16_FLOAT:
      return &load_r16g16_float;
   case DXGI_FORMAT_R16G16_UNORM:
      return &load_r16g16_unorm;
   case DXGI_FORMAT_R16G16_SNORM:
      return &load_r16g16_snorm;
   case DXGI_FORMAT_R16_FLOAT:
      return &load_r16_float;
   case DXGI_FORMAT_R16_UNORM:
      return &load_r16_unorm;
   case DXGI_FORMAT_R16_SNORM:
      return &load_r16_snorm;
   case DXGI_FORMAT_R11G11B10_FLOAT:
      return &load_r11g11b10_float;
   case DXGI_FORMAT_R10G10B10A2_UNORM:
      return &load_r10g10b10a2_unorm;
   case DXGI_FORMAT_R8G8B8A8_UNORM:
      return &load_r8g8b8a8_unorm;
   case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
      return &load_r8g8b8a8_unorm_srgb;
   case DXGI_FORMAT_R8G8B8A8_SNORM:
      return &load_r8g8b8a8_snorm;
   case DXGI_FORMAT_R8G8_UNORM:
      return &load_r8g8_unorm;
   case DXGI_FORMAT_R8G8_SNORM:
      return &load_r8g8_snorm;
   case DXGI_FORMAT_R8_UNORM:
      return &load_r8_unorm;
   case DXGI_FORMAT_R8_SNORM:
      return &load_r8_snorm;
   case DXGI_FORMAT_A8_UNORM:
      return &load_a8_unorm;
   case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
      return &load_r9g9b9e5_float;
   case DXGI_FORMAT_B5G6R5_UNORM:
      return &load_b5g6r5_unorm;
   case DXGI_FORMAT_B5G5R5A1_UNORM:
      return &load_b5g5r5a1_unorm;
   case DXGI_FORMAT_B8G8R8A8_UNORM:
      return &load_b8g8r8a8_unorm;
   case DXGI_FORMAT_B8G8R8X8_UNORM:
      return &load_b8g8r8x8_unorm;
   case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
      return &load_b8g8r8a8_unorm_srgb;
   case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
      return &load_b8g8r8x8_unorm_srgb;
   case DXGI_FORMAT_B4G4R4A4_UNORM:
      return &load_b4g4r4a4_unorm;
   default:
      std::terminate();
   }
}

auto get_store_function(const DXGI_FORMAT format) noexcept
{
   switch (format) {
   case DXGI_FORMAT_R32G32B32A32_FLOAT:
      return &store_r32g32b32a32_float;
   case DXGI_FORMAT_R32G32B32_FLOAT:
      return &store_r32g32b32_float;
   case DXGI_FORMAT_R32G32_FLOAT:
      return &store_r32g32_float;
   case DXGI_FORMAT_R32_FLOAT:
      return &store_r32_float;
   case DXGI_FORMAT_R16G16B16A16_FLOAT:
      return &store_r16g16b16a16_float;
   case DXGI_FORMAT_R16G16B16A16_UNORM:
      return &store_r16g16b16a16_unorm;
   case DXGI_FORMAT_R16G16B16A16_SNORM:
      return &store_r16g16b16a16_snorm;
   case DXGI_FORMAT_R16G16_FLOAT:
      return &store_r16g16_float;
   case DXGI_FORMAT_R16G16_UNORM:
      return &store_r16g16_unorm;
   case DXGI_FORMAT_R16G16_SNORM:
      return &store_r16g16_snorm;
   case DXGI_FORMAT_R16_FLOAT:
      return &store_r16_float;
   case DXGI_FORMAT_R16_UNORM:
      return &store_r16_unorm;
   case DXGI_FORMAT_R16_SNORM:
      return &store_r16_snorm;
   case DXGI_FORMAT_R11G11B10_FLOAT:
      return &store_r11g11b10_float;
   case DXGI_FORMAT_R10G10B10A2_UNORM:
      return &store_r10g10b10a2_unorm;
   case DXGI_FORMAT_R8G8B8A8_UNORM:
      return &store_r8g8b8a8_unorm;
   case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
      return &store_r8g8b8a8_unorm_srgb;
   case DXGI_FORMAT_R8G8B8A8_SNORM:
      return &store_r8g8b8a8_snorm;
   case DXGI_FORMAT_R8G8_UNORM:
      return &store_r8g8_unorm;
   case DXGI_FORMAT_R8G8_SNORM:
      return &store_r8g8_snorm;
   case DXGI_FORMAT_R8_UNORM:
      return &store_r8_unorm;
   case DXGI_FORMAT_R8_SNORM:
      return &store_r8_snorm;
   case DXGI_FORMAT_A8_UNORM:
      return &store_a8_unorm;
   case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
      return &store_r9g9b9e5_float;
   case DXGI_FORMAT_B5G6R5_UNORM:
      return &store_b5g6r5_unorm;
   case DXGI_FORMAT_B5G5R5A1_UNORM:
      return &store_b5g5r5a1_unorm;
   case DXGI_FORMAT_B8G8R8A8_UNORM:
      return &store_b8g8r8a8_unorm;
   case DXGI_FORMAT_B8G8R8X8_UNORM:
      return &store_b8g8r8x8_unorm;
   case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
      return &store_b8g8r8a8_unorm_srgb;
   case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
      return &store_b8g8r8x8_unorm_srgb;
   case DXGI_FORMAT_B4G4R4A4_UNORM:
      return &store_b4g4r4a4_unorm;
   default:
      std::terminate();
   }
}

auto get_bytes_per_pixel(const DXGI_FORMAT format) noexcept -> std::size_t
{
   switch (format) {
   case DXGI_FORMAT_R32G32B32A32_FLOAT:
      return sizeof(glm::vec4);
   case DXGI_FORMAT_R32G32B32_FLOAT:
      return sizeof(glm::vec3);
   case DXGI_FORMAT_R32G32_FLOAT:
      return sizeof(glm::vec2);
   case DXGI_FORMAT_R32_FLOAT:
      return sizeof(glm::vec1);
   case DXGI_FORMAT_R16G16B16A16_FLOAT:
      return sizeof(glm::uint64);
   case DXGI_FORMAT_R16G16B16A16_UNORM:
      return sizeof(glm::uint64);
   case DXGI_FORMAT_R16G16B16A16_SNORM:
      return sizeof(glm::uint64);
   case DXGI_FORMAT_R16G16_FLOAT:
      return sizeof(glm::uint32);
   case DXGI_FORMAT_R16G16_UNORM:
      return sizeof(glm::uint32);
   case DXGI_FORMAT_R16G16_SNORM:
      return sizeof(glm::uint32);
   case DXGI_FORMAT_R16_FLOAT:
   case DXGI_FORMAT_R16_UNORM:
   case DXGI_FORMAT_R16_SNORM:
      return sizeof(glm::uint16);
   case DXGI_FORMAT_R11G11B10_FLOAT:
   case DXGI_FORMAT_R10G10B10A2_UNORM:
   case DXGI_FORMAT_R8G8B8A8_UNORM:
   case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
   case DXGI_FORMAT_R8G8B8A8_SNORM:
      return sizeof(glm::uint32);
   case DXGI_FORMAT_R8G8_UNORM:
   case DXGI_FORMAT_R8G8_SNORM:
      return sizeof(glm::uint16);
   case DXGI_FORMAT_R8_UNORM:
   case DXGI_FORMAT_R8_SNORM:
   case DXGI_FORMAT_A8_UNORM:
      return sizeof(glm::uint8);
   case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
      return sizeof(glm::uint32);
   case DXGI_FORMAT_B5G6R5_UNORM:
   case DXGI_FORMAT_B5G5R5A1_UNORM:
      return sizeof(glm::uint16);
   case DXGI_FORMAT_B8G8R8A8_UNORM:
   case DXGI_FORMAT_B8G8R8X8_UNORM:
   case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
   case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
      return sizeof(glm::uint32);
   case DXGI_FORMAT_B4G4R4A4_UNORM:
      return sizeof(glm::uint16);
   default:
      std::terminate();
   }
}
}

Image_span::Image_span(const glm::ivec2 size, const std::size_t row_pitch,
                       std::byte* const data, const DXGI_FORMAT format) noexcept
   : _bounds{size - 1},
     _row_pitch{row_pitch},
     _data{data},
     _load_func{*get_load_function(format)},
     _store_func{*get_store_function(format)},
     _format{format}
{
}

Image_span::Image_span(const DirectX::Image& image) noexcept
   : Image_span{{image.width, image.height},
                image.rowPitch,
                reinterpret_cast<std::byte*>(image.pixels),
                image.format}
{
}

auto Image_span::load(const index_type index) const noexcept -> value_type
{
   const auto clamped_index = glm::clamp(index, glm::ivec2{0}, _bounds);

   return _load_func(clamped_index, _row_pitch, _data);
}

void Image_span::store(const index_type index, const value_type value) noexcept
{
   const auto clamped_index = glm::clamp(index, glm::ivec2{0}, _bounds);

   _store_func(value, clamped_index, _row_pitch, _data);
}

auto Image_span::exchange(const index_type index, const value_type new_value) noexcept
   -> value_type
{
   const auto clamped_index = glm::clamp(index, glm::ivec2{0}, _bounds);

   const auto old_value = _load_func(clamped_index, _row_pitch, _data);

   _store_func(new_value, clamped_index, _row_pitch, _data);

   return old_value;
}

auto Image_span::subspan(const index_type offset, const index_type length) const
   noexcept -> Image_span
{
   Expects(glm::all(glm::lessThanEqual((offset + length), size())));

   const auto bytes_per_pixel = get_bytes_per_pixel(_format);

   const auto x_offset = bytes_per_pixel * offset.x;
   const auto sub_row_pitch = _row_pitch + x_offset;
   auto* const sub_data = _data + (_row_pitch * offset.y) + x_offset;

   return {length, sub_row_pitch, sub_data, _format};
}

auto Image_span::subspan_rows(const index_type::value_type row,
                              const index_type::value_type count) const noexcept -> Image_span
{
   Expects((row + count) <= size().y);

   auto* const sub_data = _data + (_row_pitch * row);

   return {{size().x, count}, _row_pitch, sub_data, _format};
}

auto Image_span::size() const noexcept -> glm::ivec2
{
   return _bounds + 1;
}
}
