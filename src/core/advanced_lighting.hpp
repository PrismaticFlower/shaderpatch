#pragma once

#include "lighting/matrices.hpp"

#include "../game_support/light_list.hpp"

#include "com_ptr.hpp"

#include <glm/glm.hpp>

#include <d3d11_4.h>

namespace sp::shadows {
struct Frustum;
}

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

   std::vector<game_support::Light_texture> textures;
   std::vector<glm::mat4> visible_textures;

   constexpr static float x_clusters = 16.0f;
   constexpr static float y_clusters = 8.0f;
   constexpr static std::size_t texture_count_per_type = 3;

   void update(ID3D11DeviceContext4& dc, const Light_cluster_matrices& matrices);

   auto get_srvs() const noexcept -> std::array<ID3D11ShaderResourceView*, 11>;

private:
   struct Sorting_texture {
      std::uint32_t texture_name_hash = 0;
      float nearest_light = FLT_MAX;
   };

   struct Light_texture {
      std::uint32_t texture_name_hash = 0;
      Com_ptr<ID3D11ShaderResourceView> srv;
   };

   template<typename T>
   void apply_textures(std::vector<T>& visible_lights,
                       const std::array<Light_texture, texture_count_per_type>& active_light_textures) noexcept;

   void process_textures(const shadows::Frustum& frustum,
                         const bool srgb_textures) noexcept;

   static auto select_textures(std::span<const Sorting_texture> light_textures,
                               const bool srgb_textures) noexcept
      -> std::array<Light_texture, texture_count_per_type>;

   void upload_light_buffers(ID3D11DeviceContext4& dc) noexcept;

   std::vector<Spot_light_bounds> _spot_lights_bounds;

   std::vector<Sorting_texture> _light_textures_scratch;

   std::array<Light_texture, texture_count_per_type> _active_point_light_textures;
   std::array<Light_texture, texture_count_per_type> _active_spot_light_textures;

   Com_ptr<ID3D11Device5> _device;

   std::unique_ptr<Light_clusters> _clusters;

   std::uint32_t _point_lights_capacity = 0;
   Com_ptr<ID3D11Buffer> _point_lights_buffer;
   Com_ptr<ID3D11ShaderResourceView> _point_lights_srv;

   std::uint32_t _spot_lights_capacity = 0;
   Com_ptr<ID3D11Buffer> _spot_lights_buffer;
   Com_ptr<ID3D11ShaderResourceView> _spot_lights_srv;

   std::uint32_t _textures_capacity = 0;
   Com_ptr<ID3D11Buffer> _textures_buffer;
   Com_ptr<ID3D11ShaderResourceView> _textures_srv;
};

}
