#pragma once

#include "../shader/database.hpp"
#include "bounding_box.hpp"
#include "com_ptr.hpp"
#include "shader_input_layouts.hpp"

#include <array>
#include <limits>
#include <vector>

#include <d3d11_4.h>

#include <absl/container/flat_hash_map.h>
#include <glm/glm.hpp>

namespace sp::effects {
class Profiler;
}

namespace sp::core {

struct Shadows_config {
   float start_depth = 0.5f;
   float end_depth = 256.0f;
   float shadow_bias = -0.0015f;
};

class Shadows_provider {
public:
   struct Meshes {
      constexpr static UINT noskin = std::numeric_limits<UINT>::max();

      struct Zprepass {
         D3D11_PRIMITIVE_TOPOLOGY primitive_topology;
         Com_ptr<ID3D11Buffer> index_buffer;
         UINT index_buffer_offset;
         Com_ptr<ID3D11Buffer> vertex_buffer;
         UINT vertex_buffer_offset;
         UINT vertex_buffer_stride;
         UINT index_count;
         UINT start_index;
         UINT start_vertex;
         glm::vec3 position_decompress_min;
         glm::vec3 position_decompress_max;
         std::array<glm::vec4, 3> world_matrix;
         UINT skin_index = noskin;

         template<typename H>
         friend H AbslHashValue(H h, const Zprepass& mesh)
         {
            return H::combine(std::move(h), mesh.primitive_topology,
                              mesh.index_buffer.get(), mesh.index_buffer_offset,
                              mesh.vertex_buffer.get(), mesh.vertex_buffer_offset,
                              mesh.vertex_buffer_stride, mesh.index_count,
                              mesh.start_index, mesh.start_vertex,
                              mesh.position_decompress_min[0],
                              mesh.position_decompress_min[1],
                              mesh.position_decompress_min[2],
                              mesh.position_decompress_max[0],
                              mesh.position_decompress_max[1],
                              mesh.position_decompress_max[2],
                              mesh.world_matrix[0][0], mesh.world_matrix[0][1],
                              mesh.world_matrix[0][2], mesh.world_matrix[0][3],
                              mesh.world_matrix[1][0], mesh.world_matrix[1][1],
                              mesh.world_matrix[1][2], mesh.world_matrix[1][3],
                              mesh.world_matrix[2][0], mesh.world_matrix[2][1],
                              mesh.world_matrix[2][2], mesh.world_matrix[2][3]
                              //, mesh.skin_index // exclude skin_index as we never cache skinned meshes
            );
         }

         bool operator==(const Zprepass&) const noexcept = default;
      };

      struct Zprepass_hardedged {
         std::int16_t input_layout;
         D3D11_PRIMITIVE_TOPOLOGY primitive_topology;
         Com_ptr<ID3D11Buffer> index_buffer;
         UINT index_buffer_offset;
         Com_ptr<ID3D11Buffer> vertex_buffer;
         UINT vertex_buffer_offset;
         UINT vertex_buffer_stride;
         UINT index_count;
         UINT start_index;
         UINT start_vertex;
         glm::vec3 position_decompress_min;
         glm::vec3 position_decompress_max;
         std::array<glm::vec4, 3> world_matrix;
         UINT skin_index = noskin;
         glm::vec4 x_texcoord_transform;
         glm::vec4 y_texcoord_transform;
         Com_ptr<ID3D11ShaderResourceView> texture;

         template<typename H>
         friend H AbslHashValue(H h, const Zprepass_hardedged& mesh)
         {
            return H::combine(
               std::move(h), mesh.primitive_topology, mesh.index_buffer.get(),
               mesh.index_buffer_offset, mesh.vertex_buffer.get(),
               mesh.vertex_buffer_offset, mesh.vertex_buffer_stride,
               mesh.index_count, mesh.start_index, mesh.start_vertex,
               mesh.position_decompress_min[0], mesh.position_decompress_min[1],
               mesh.position_decompress_min[2], mesh.position_decompress_max[0],
               mesh.position_decompress_max[1], mesh.position_decompress_max[2],
               mesh.world_matrix[0][0], mesh.world_matrix[0][1],
               mesh.world_matrix[0][2], mesh.world_matrix[0][3],
               mesh.world_matrix[1][0], mesh.world_matrix[1][1],
               mesh.world_matrix[1][2], mesh.world_matrix[1][3],
               mesh.world_matrix[2][0], mesh.world_matrix[2][1],
               mesh.world_matrix[2][2], mesh.world_matrix[2][3],
               //, mesh.skin_index // exclude skin_index as we never cache skinned meshes
               mesh.x_texcoord_transform[0], mesh.x_texcoord_transform[1],
               mesh.x_texcoord_transform[2], mesh.x_texcoord_transform[3],
               mesh.y_texcoord_transform[0], mesh.y_texcoord_transform[1],
               mesh.y_texcoord_transform[2], mesh.y_texcoord_transform[3],
               mesh.texture.get());
         }

         bool operator==(const Zprepass_hardedged&) const noexcept = default;
      };

      struct Skin {
         std::array<std::array<glm::vec4, 3>, 15> bone_matrices;
      };

      std::vector<Zprepass> zprepass = [] {
         std::vector<Zprepass> init;
         init.reserve(128);

         return init;
      }();

      std::vector<Zprepass> zprepass_compressed = [] {
         std::vector<Zprepass> init;
         init.reserve(1024);

         return init;
      }();

      std::vector<Zprepass> zprepass_skinned = [] {
         std::vector<Zprepass> init;
         init.reserve(16);

         return init;
      }();

      std::vector<Zprepass> zprepass_compressed_skinned = [] {
         std::vector<Zprepass> init;
         init.reserve(256);

         return init;
      }();

      std::vector<Zprepass_hardedged> zprepass_hardedged = [] {
         std::vector<Zprepass_hardedged> init;
         init.reserve(128);

         return init;
      }();

      std::vector<Zprepass_hardedged> zprepass_hardedged_compressed = [] {
         std::vector<Zprepass_hardedged> init;
         init.reserve(128);

         return init;
      }();

      std::vector<Zprepass_hardedged> zprepass_hardedged_skinned = [] {
         std::vector<Zprepass_hardedged> init;
         init.reserve(4);

         return init;
      }();

      std::vector<Zprepass_hardedged> zprepass_hardedged_compressed_skinned = [] {
         std::vector<Zprepass_hardedged> init;
         init.reserve(4);

         return init;
      }();

      std::vector<Skin> skins = [] {
         std::vector<Skin> init;
         init.reserve(512);

         return init;
      }();

      auto select_zprepass(const bool compressed, const bool skinned) noexcept
         -> std::vector<Zprepass>&
      {
         if (compressed) {
            if (skinned) {
               return zprepass_compressed_skinned;
            }
            else {
               return zprepass_compressed;
            }
         }
         else {
            if (skinned) {
               return zprepass_skinned;
            }
            else {
               return zprepass;
            }
         }
      }

      auto select_zprepass_hardedged(const bool compressed, const bool skinned) noexcept
         -> std::vector<Zprepass_hardedged>&
      {
         if (compressed) {
            if (skinned) {
               return zprepass_hardedged_compressed_skinned;
            }
            else {
               return zprepass_hardedged_compressed;
            }
         }
         else {
            if (skinned) {
               return zprepass_hardedged_skinned;
            }
            else {
               return zprepass_hardedged;
            }
         }
      }

      void clear() noexcept
      {
         zprepass.clear();
         zprepass_compressed.clear();
         zprepass_skinned.clear();
         zprepass_compressed_skinned.clear();
         zprepass_hardedged.clear();
         zprepass_hardedged_compressed.clear();
         zprepass_hardedged_skinned.clear();
         zprepass_hardedged_compressed_skinned.clear();
         skins.clear();
      }
   };

   Meshes meshes;

   glm::mat4 view_proj_matrix;
   glm::vec3 light_direction = {1.0f, 0.0f, 0.0f};

   Shadows_config config;

   Shadows_provider(Com_ptr<ID3D11Device5> device, shader::Database& shaders) noexcept;

   void end_frame() noexcept;

   struct Draw_args {
      const Input_layout_descriptions& input_layout_descriptions;
      ID3D11ShaderResourceView& scene_depth;
      ID3D11DepthStencilView& scene_dsv;
      ID3D11RenderTargetView& target_map;
      UINT target_width;
      UINT target_height;
      effects::Profiler& profiler;
   };

   void draw_shadow_maps(ID3D11DeviceContext4& dc, const Draw_args& args) noexcept;

   std::size_t temp_cached_zprepass_compressed = 0;
   std::size_t temp_cached_zprepass_hardedged_compressed = 0;

private:
   void prepare_draw_shadow_maps(ID3D11DeviceContext4& dc, const Draw_args& args) noexcept;

   void clean_mesh_cache() noexcept;

   void populate_mesh_cache() noexcept;

   void build_cascade_info() noexcept;

   void upload_buffer_data(ID3D11DeviceContext4& dc, const Draw_args& args) noexcept;

   void upload_camera_cb_buffer(ID3D11DeviceContext4& dc) noexcept;

   void upload_transform_cb_buffer(ID3D11DeviceContext4& dc) noexcept;

   void upload_skins_buffer(ID3D11DeviceContext4& dc) noexcept;

   void upload_draw_to_target_buffer(ID3D11DeviceContext4& dc,
                                     const Draw_args& args) noexcept;

   void resize_transform_cb_buffer(std::size_t needed_space) noexcept;

   void resize_skins_buffer(std::size_t needed_space) noexcept;

   auto count_active_meshes() const noexcept -> std::size_t;

   void create_shadow_map(const UINT length) noexcept;

   void draw_shadow_maps_instanced(ID3D11DeviceContext4& dc,
                                   const Input_layout_descriptions& input_layout_descriptions,
                                   effects::Profiler& profiler) noexcept;

   void draw_to_target_map(ID3D11DeviceContext4& dc, const Draw_args& args) noexcept;

   struct Meshes_cache {
      constexpr static UINT noskin = Meshes::noskin;

      using Zprepass = Meshes::Zprepass;
      using Zprepass_hardedged = Meshes::Zprepass_hardedged;

      template<typename T>
      struct Entry {
         Bounding_box bbox;
         T* cached_mesh;
      };

      absl::flat_hash_map<Zprepass, Bounding_box> zprepass_compressed = [] {
         absl::flat_hash_map<Zprepass, Bounding_box> init;

         init.reserve(1024);

         return init;
      }();
      absl::flat_hash_map<Zprepass_hardedged, Bounding_box> zprepass_hardedged_compressed =
         [] {
            absl::flat_hash_map<Zprepass_hardedged, Bounding_box> init;

            init.reserve(1024);

            return init;
         }();

      std::size_t dynamic_mesh_data_cache_frame = 0;
      std::array<Com_ptr<ID3D11Buffer>, 2> dynamic_mesh_data_cache;
   };

   struct Hardedged_vertex_shader {
      Hardedged_vertex_shader(
         std::tuple<Com_ptr<ID3D11VertexShader>, shader::Bytecode_blob, shader::Vertex_input_layout> shader);

      Com_ptr<ID3D11VertexShader> vs;
      Shader_input_layouts input_layouts;
   };

   Com_ptr<ID3D11Device5> _device;

   Com_ptr<ID3D11Buffer> _camera_cb_buffer;

   Com_ptr<ID3D11Buffer> _transforms_cb_buffer;
   std::size_t _transforms_cb_space = 0;
   Com_ptr<ID3D11Buffer> _skins_buffer;
   std::size_t _skins_space = 0;
   Com_ptr<ID3D11ShaderResourceView> _skins_srv;

   Com_ptr<ID3D11InputLayout> _mesh_il;
   Com_ptr<ID3D11InputLayout> _mesh_compressed_il;
   Com_ptr<ID3D11InputLayout> _mesh_skinned_il;
   Com_ptr<ID3D11InputLayout> _mesh_compressed_skinned_il;

   Com_ptr<ID3D11VertexShader> _mesh_vs;
   Com_ptr<ID3D11VertexShader> _mesh_compressed_vs;
   Com_ptr<ID3D11VertexShader> _mesh_skinned_vs;
   Com_ptr<ID3D11VertexShader> _mesh_compressed_skinned_vs;

   Hardedged_vertex_shader _mesh_hardedged;
   Hardedged_vertex_shader _mesh_hardedged_compressed;
   Hardedged_vertex_shader _mesh_hardedged_skinned;
   Hardedged_vertex_shader _mesh_hardedged_compressed_skinned;

   Com_ptr<ID3D11PixelShader> _mesh_hardedged_ps;

   Com_ptr<ID3D11SamplerState> _hardedged_texture_sampler;

   Com_ptr<ID3D11Texture2D> _shadow_map_texture;
   Com_ptr<ID3D11DepthStencilView> _shadow_map_dsv;
   Com_ptr<ID3D11ShaderResourceView> _shadow_map_srv;

   float _shadow_map_length_flt = 0.0f;

   Com_ptr<ID3D11RasterizerState> _rasterizer_state;
   Com_ptr<ID3D11RasterizerState> _rasterizer_doublesided_state;

   std::array<glm::mat4, 4> _cascade_view_proj_matrices;
   std::array<glm::mat4, 4> _cascade_texture_matrices;

   Com_ptr<ID3D11VertexShader> _draw_to_target_vs;
   Com_ptr<ID3D11PixelShader> _draw_to_target_ps;
   Com_ptr<ID3D11Buffer> _draw_to_target_cb;
   Com_ptr<ID3D11DepthStencilState> _draw_to_target_depthstencil_state;
   Com_ptr<ID3D11SamplerState> _shadow_map_sampler;

   Meshes_cache _meshes_cache;
};

}