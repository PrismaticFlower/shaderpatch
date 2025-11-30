#pragma once

#include "../../game_support/light_list.hpp"

#include "matrices.hpp"

#include "com_ptr.hpp"

#include <array>
#include <span>
#include <vector>

#include <d3d11_4.h>

namespace sp::core {

struct Light_clusters {
   explicit Light_clusters(ID3D11Device5& device) noexcept;

   constexpr static UINT cluster_max_lights = 128;

   void build(const Light_cluster_matrices& matrices,
              std::span<const game_support::Point_light> point_lights,
              std::span<const game_support::Spot_light> spot_lights) noexcept;

   void upload(ID3D11DeviceContext4& dc) noexcept;

   Com_ptr<ID3D11ShaderResourceView> cluster_index_srv;
   Com_ptr<ID3D11ShaderResourceView> cluster_light_lists_srv;

private:
   Com_ptr<ID3D11Buffer> _cluster_index_buffer;
   Com_ptr<ID3D11Buffer> _cluster_light_lists_buffer;

   constexpr static int _x_clusters = 16;
   constexpr static int _y_clusters = 8;
   constexpr static int _z_clusters = 32;

   struct Spot_light_bounds {
      glm::vec3 positionVS = {};

      float range = 0.0f;

      glm::vec3 directionVS = {};

      float sin_angle = 0.0f;
      float cos_angle = 0.0f;

      glm::vec3 minVS = {};
      glm::vec3 maxVS = {};
   };

   struct Z_bin {
      std::vector<std::uint16_t> point_lights;
      std::vector<std::uint16_t> spot_lights;
   };

   struct Cluster_bounds {
      glm::vec3 centreVS;
      glm::vec3 sizeVS;
   };

   struct Cluster_info {
      std::uint8_t point_light_count = 0;
      std::uint8_t spot_light_count = 0;
   };

   struct Cluster {
      Cluster_info info;
      std::array<std::uint32_t, cluster_max_lights> lights;
   };

   std::vector<Spot_light_bounds> _spot_light_bounds;

   std::array<Z_bin, _z_clusters> _z_bins;
   std::array<std::array<std::array<Cluster, _x_clusters>, _y_clusters>, _z_clusters> _xy_clusters;

   std::array<std::array<glm::vec3, _x_clusters + 1>, _y_clusters + 1> _tile_corner_directions;
   std::array<float, _z_clusters + 1> _z_splits;
   std::array<std::array<std::array<Cluster_bounds, _x_clusters>, _y_clusters>, _z_clusters> _cluster_bounds;

   void build_spot_light_bounds(const Light_cluster_matrices& matrices,
                                std::span<const game_support::Spot_light> spot_lights) noexcept;

   void z_bin_lights(const Light_cluster_matrices& matrices,
                     std::span<const game_support::Point_light> point_lights,
                     std::span<const game_support::Spot_light> spot_lights) noexcept;

   void build_tile_directions(const Light_cluster_matrices& matrices) noexcept;

   void build_cluster_bounds() noexcept;

   void build_xy_clusters(const Light_cluster_matrices& matrices,
                          std::span<const game_support::Point_light> point_lights,
                          std::span<const game_support::Spot_light> spot_lights) noexcept;
};

}