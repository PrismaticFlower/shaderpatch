#pragma once

#include "lighting/matrices.hpp"

#include "../game_support/light_list.hpp"

#include "com_ptr.hpp"

#include <glm/glm.hpp>

#include <d3d11_4.h>

namespace sp::core {

struct Light_clusters;
struct Spot_light_bounds;

struct Advanced_lighting {
   explicit Advanced_lighting(ID3D11Device5& device) noexcept;

   ~Advanced_lighting();

   glm::vec3 global_light1_dir;

   std::vector<game_support::Point_light> point_lights;
   std::vector<game_support::Point_light> visible_point_lights;

   std::vector<game_support::Spot_light> spot_lights;
   std::vector<game_support::Spot_light> visible_spot_lights;

   void update(ID3D11DeviceContext4& dc, const Light_cluster_matrices& matrices);

   auto get_srvs() const noexcept -> std::array<ID3D11ShaderResourceView*, 4>;

private:
   void upload_light_buffers(ID3D11DeviceContext4& dc) noexcept;

   std::vector<Spot_light_bounds> _spot_lights_bounds;

   Com_ptr<ID3D11Device5> _device;

   std::unique_ptr<Light_clusters> _clusters;

   std::uint32_t _point_lights_capacity = 0;
   Com_ptr<ID3D11Buffer> _point_lights_buffer;
   Com_ptr<ID3D11ShaderResourceView> _point_lights_srv;

   std::uint32_t _spot_lights_capacity = 0;
   Com_ptr<ID3D11Buffer> _spot_lights_buffer;
   Com_ptr<ID3D11ShaderResourceView> _spot_lights_srv;
};

}
