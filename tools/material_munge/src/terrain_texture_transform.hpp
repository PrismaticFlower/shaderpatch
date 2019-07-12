#pragma once

#include <array>
#include <cstdint>

#include <glm/glm.hpp>

namespace sp {

enum class Terrain_texture_axis : std::uint8_t {
   xz,
   xy,
   yz,
   zx,
   yx,
   zy,
   negative_xz,
   negative_xy,
   negative_yz,
   negative_zx,
   negative_yx,
   negative_zy
};

class Terrain_texture_transform {
public:
   void set_axis(const Terrain_texture_axis axis) noexcept
   {
      _axis = axis;

      update();
   }

   auto get_axis() const noexcept -> Terrain_texture_axis
   {
      return _axis;
   }

   void set_scale(const float scale) noexcept
   {
      _scale = scale;

      update();
   }

   auto get_scale() const noexcept -> float
   {
      return _scale;
   }

   void set_rotation(const float rotation) noexcept
   {
      _rotation = rotation;

      update();
   }

   auto get_rotation() const noexcept -> float
   {
      return _rotation;
   }

   auto get_transform() const noexcept -> glm::mat2x3
   {
      return _transform;
   }

   auto apply(const glm::vec3 position) const noexcept -> glm::vec2
   {
      return position * _transform;
   }

   auto operator()(const glm::vec3 position) const noexcept -> glm::vec2
   {
      return apply(position);
   }

private:
   auto axis_index() const noexcept -> std::array<int, 2>
   {
      switch (_axis) {
      case Terrain_texture_axis::xz:
      case Terrain_texture_axis::negative_xz:
         return {0, 2};
      case Terrain_texture_axis::xy:
      case Terrain_texture_axis::negative_xy:
         return {0, 1};
      case Terrain_texture_axis::yz:
      case Terrain_texture_axis::negative_yz:
         return {1, 2};
      case Terrain_texture_axis::zx:
      case Terrain_texture_axis::negative_zx:
         return {2, 0};
      case Terrain_texture_axis::yx:
      case Terrain_texture_axis::negative_yx:
         return {1, 0};
      case Terrain_texture_axis::zy:
      case Terrain_texture_axis::negative_zy:
         return {2, 1};
      default:
         return {0, 0};
      }
   }

   auto axis_sign() const noexcept -> float
   {
      switch (_axis) {
      case Terrain_texture_axis::xz:
      case Terrain_texture_axis::xy:
      case Terrain_texture_axis::yz:
      case Terrain_texture_axis::zx:
      case Terrain_texture_axis::yx:
      case Terrain_texture_axis::zy:
         return 1.0f;
      case Terrain_texture_axis::negative_xz:
      case Terrain_texture_axis::negative_xy:
      case Terrain_texture_axis::negative_yz:
      case Terrain_texture_axis::negative_zx:
      case Terrain_texture_axis::negative_yx:
      case Terrain_texture_axis::negative_zy:
         return -1.0f;
      default:
         return 1.0f;
      }
   }

   void update() noexcept
   {
      const auto [x, y] = axis_index();
      const auto sign = axis_sign();

      _transform = {};

      _transform[0][x] = glm::cos(_rotation);
      _transform[0][y] = -glm::sin(_rotation);

      _transform[1][x] = glm::sin(_rotation);
      _transform[1][y] = glm::cos(_rotation);

      _transform *= (sign * (1.0f / _scale));
   }

   float _scale{};
   float _rotation{};

   glm::mat2x3 _transform{};

   Terrain_texture_axis _axis{};
};
}
