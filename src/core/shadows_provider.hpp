#pragma once

#include "../shader/database.hpp"
#include "com_ptr.hpp"
#include "shader_input_layouts.hpp"

#include <array>
#include <limits>
#include <vector>

#include <d3d11_4.h>

#include <glm/glm.hpp>

namespace sp::effects {
class Profiler;
}

namespace sp::shadows {
struct Bounding_sphere;
}

namespace sp::core {

struct Shadows_config {
   bool disable_dynamic_hardedged_meshes = false;

   INT hw_depth_bias = 0;
   float hw_depth_bias_clamp = 0.0f;
   float hw_slope_scaled_depth_bias = 0.0f;

   float start_depth = 0.5f;
   float end_depth = 256.0f;
   float shadow_bias = -0.0015f;
};

class Shadows_provider {
public:
   glm::mat4 view_proj_matrix;
   glm::vec3 light_direction = {1.0f, 0.0f, 0.0f};

   Shadows_config config;

   Shadows_provider(Com_ptr<ID3D11Device5> device, shader::Database& shaders,
                    const Input_layout_descriptions& input_layout_descriptions) noexcept;

   ~Shadows_provider();

   struct Input_mesh {
      D3D11_PRIMITIVE_TOPOLOGY primitive_topology;

      ID3D11Buffer& index_buffer;
      UINT index_buffer_offset;

      ID3D11Buffer& vertex_buffer;
      UINT vertex_buffer_offset;
      UINT vertex_buffer_stride;

      UINT index_count;
      UINT start_index;
      INT base_vertex;

      const std::array<glm::vec4, 3>& world_matrix;
   };

   struct Input_mesh_compressed {
      D3D11_PRIMITIVE_TOPOLOGY primitive_topology;

      ID3D11Buffer& index_buffer;
      UINT index_buffer_offset;

      ID3D11Buffer& vertex_buffer;
      UINT vertex_buffer_offset;
      UINT vertex_buffer_stride;

      UINT index_count;
      UINT start_index;
      INT base_vertex;

      const std::array<glm::vec4, 3>& world_matrix;

      const glm::vec4& position_decompress_mul;
      const glm::vec4& position_decompress_add;
   };

   struct Input_hardedged_mesh {
      std::uint16_t input_layout;

      D3D11_PRIMITIVE_TOPOLOGY primitive_topology;

      ID3D11Buffer& index_buffer;
      UINT index_buffer_offset;

      ID3D11Buffer& vertex_buffer;
      UINT vertex_buffer_offset;
      UINT vertex_buffer_stride;

      UINT index_count;
      UINT start_index;
      INT base_vertex;

      const std::array<glm::vec4, 3>& world_matrix;

      const glm::vec4& x_texcoord_transform;
      const glm::vec4& y_texcoord_transform;
      ID3D11ShaderResourceView& texture;
   };

   struct Input_mesh_hardedged_compressed {
      std::uint16_t input_layout;

      D3D11_PRIMITIVE_TOPOLOGY primitive_topology;

      ID3D11Buffer& index_buffer;
      UINT index_buffer_offset;

      ID3D11Buffer& vertex_buffer;
      UINT vertex_buffer_offset;
      UINT vertex_buffer_stride;

      UINT index_count;
      UINT start_index;
      INT base_vertex;

      const std::array<glm::vec4, 3>& world_matrix;

      const glm::vec4& x_texcoord_transform;
      const glm::vec4& y_texcoord_transform;
      ID3D11ShaderResourceView& texture;

      const glm::vec4& position_decompress_mul;
      const glm::vec4& position_decompress_add;
   };

   void add_mesh(ID3D11DeviceContext4& dc, const Input_mesh& mesh);

   void add_mesh_compressed(ID3D11DeviceContext4& dc, const Input_mesh_compressed& mesh);

   void add_mesh_skinned(ID3D11DeviceContext4& dc, const Input_mesh& mesh,
                         const std::array<std::array<glm::vec4, 3>, 15>& bone_matrices);

   void add_mesh_compressed_skinned(
      ID3D11DeviceContext4& dc, const Input_mesh_compressed& mesh,
      const std::array<std::array<glm::vec4, 3>, 15>& bone_matrices);

   void add_mesh_hardedged(ID3D11DeviceContext4& dc, const Input_hardedged_mesh& mesh);

   void add_mesh_hardedged_compressed(ID3D11DeviceContext4& dc,
                                      const Input_mesh_hardedged_compressed& mesh);

   void add_mesh_hardedged_skinned(ID3D11DeviceContext4& dc,
                                   const Input_hardedged_mesh& mesh,
                                   const std::array<std::array<glm::vec4, 3>, 15>& bone_matrices);

   void add_mesh_hardedged_compressed_skinned(
      ID3D11DeviceContext4& dc, const Input_mesh_hardedged_compressed& mesh,
      const std::array<std::array<glm::vec4, 3>, 15>& bone_matrices);

   struct Draw_args {
      ID3D11ShaderResourceView& scene_depth;
      ID3D11DepthStencilView& scene_dsv;
      ID3D11RenderTargetView& target_map;
      UINT target_width;
      UINT target_height;
      effects::Profiler& profiler;
   };

   void draw_shadow_maps(ID3D11DeviceContext4& dc, const Draw_args& args) noexcept;

   void end_frame(ID3D11DeviceContext4& dc) noexcept;

private:
   struct alignas(256) Transform_cb {
      glm::vec3 position_decompress_mul;
      std::uint32_t skin_index;
      glm::vec3 position_decompress_add;
      std::uint32_t padding;
      std::array<glm::vec4, 3> world_matrix;
      glm::vec4 x_texcoord_transform;
      glm::vec4 y_texcoord_transform;
   };

   static_assert(sizeof(Transform_cb) == 256);

   struct Mesh {
      D3D11_PRIMITIVE_TOPOLOGY primitive_topology;

      Com_ptr<ID3D11Buffer> index_buffer;
      UINT index_buffer_offset;

      Com_ptr<ID3D11Buffer> vertex_buffer;
      UINT vertex_buffer_offset;
      UINT vertex_buffer_stride;

      UINT index_count;
      UINT start_index;
      INT base_vertex;

      UINT constants_index = 0;
   };

   struct Mesh_hardedged {
      ID3D11InputLayout* input_layout;

      D3D11_PRIMITIVE_TOPOLOGY primitive_topology;

      Com_ptr<ID3D11Buffer> index_buffer;
      UINT index_buffer_offset;

      Com_ptr<ID3D11Buffer> vertex_buffer;
      UINT vertex_buffer_offset;
      UINT vertex_buffer_stride;

      UINT index_count;
      UINT start_index;
      INT base_vertex;

      UINT constants_index = 0;
      Com_ptr<ID3D11ShaderResourceView> texture;
   };

   struct Meshes {
      std::vector<Mesh> uncompressed;
      std::vector<Mesh> compressed;
      std::vector<Mesh> skinned;
      std::vector<Mesh> compressed_skinned;

      std::vector<Mesh_hardedged> hardedged;
      std::vector<Mesh_hardedged> hardedged_compressed;
      std::vector<Mesh_hardedged> hardedged_skinned;
      std::vector<Mesh_hardedged> hardedged_compressed_skinned;

      void clear() noexcept;
   };

   struct Bounding_spheres {
      Bounding_spheres();

      ~Bounding_spheres();

      std::vector<shadows::Bounding_sphere> compressed;
      std::vector<shadows::Bounding_sphere> compressed_skinned;
      std::vector<shadows::Bounding_sphere> hardedged_compressed;
      std::vector<shadows::Bounding_sphere> hardedged_compressed_skinned;

      void clear() noexcept;
   };

   struct Visibility_lists {
      std::vector<std::uint32_t> compressed;
      std::vector<std::uint32_t> compressed_skinned;
      std::vector<std::uint32_t> hardedged_compressed;
      std::vector<std::uint32_t> hardedged_compressed_skinned;

      void clear() noexcept;
   };

   Meshes _meshes;
   Bounding_spheres _bounding_spheres;
   Visibility_lists _visibility_lists;

   struct Hardedged_vertex_shader {
      Hardedged_vertex_shader(
         std::tuple<Com_ptr<ID3D11VertexShader>, shader::Bytecode_blob, shader::Vertex_input_layout> shader);

      Com_ptr<ID3D11VertexShader> vs;
      Shader_input_layouts input_layouts;
   };

   Com_ptr<ID3D11Device5> _device;

   Com_ptr<ID3D11Buffer> _camera_cb_buffer;

   Com_ptr<ID3D11Buffer> _transforms_cb_buffer;
   std::size_t _transforms_cb_space = 1024;
   std::byte* _transforms_cb_upload = nullptr;
   std::size_t _used_transforms = 0;

   Com_ptr<ID3D11Buffer> _skins_buffer;
   std::size_t _skins_space = 64;
   std::byte* _skins_buffer_upload = nullptr;
   std::size_t _used_skins = 0;
   Com_ptr<ID3D11ShaderResourceView> _skins_srv;

   bool _started_frame = false;
   bool _drawn_shadow_maps = false;

   Com_ptr<ID3D11InputLayout> _mesh_il;
   Com_ptr<ID3D11InputLayout> _mesh_compressed_il;
   Com_ptr<ID3D11InputLayout> _mesh_skinned_il;
   Com_ptr<ID3D11InputLayout> _mesh_compressed_skinned_il;
   Com_ptr<ID3D11InputLayout> _mesh_stencilshadow_skinned_il;

   Com_ptr<ID3D11VertexShader> _mesh_vs;
   Com_ptr<ID3D11VertexShader> _mesh_compressed_vs;
   Com_ptr<ID3D11VertexShader> _mesh_skinned_vs;
   Com_ptr<ID3D11VertexShader> _mesh_compressed_skinned_vs;

   const Input_layout_descriptions& _input_layout_descriptions;

   Hardedged_vertex_shader _mesh_hardedged;
   Hardedged_vertex_shader _mesh_hardedged_compressed;
   Hardedged_vertex_shader _mesh_hardedged_skinned;
   Hardedged_vertex_shader _mesh_hardedged_compressed_skinned;

   Com_ptr<ID3D11PixelShader> _mesh_hardedged_ps;

   Com_ptr<ID3D11SamplerState> _hardedged_texture_sampler;

   Com_ptr<ID3D11Texture2D> _shadow_map_texture;
   Com_ptr<ID3D11DepthStencilView> _shadow_map_dsv;
   std::array<Com_ptr<ID3D11DepthStencilView>, 4> _shadow_map_dsvs;
   Com_ptr<ID3D11ShaderResourceView> _shadow_map_srv;

   INT _current_depth_bias = config.hw_depth_bias;
   float _current_depth_bias_clamp = config.hw_depth_bias_clamp;
   float _current_slope_scaled_depth_bias = config.hw_slope_scaled_depth_bias;
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

   void begin_frame(ID3D11DeviceContext4& dc) noexcept;

   void prepare_draw_shadow_maps(ID3D11DeviceContext4& dc, const Draw_args& args) noexcept;

   void build_cascade_info() noexcept;

   void upload_buffer_data(ID3D11DeviceContext4& dc, const Draw_args& args) noexcept;

   void upload_draw_to_target_buffer(ID3D11DeviceContext4& dc,
                                     const Draw_args& args) noexcept;

   auto count_active_meshes() const noexcept -> std::size_t;

   void create_shadow_map(const UINT length) noexcept;

   void draw_shadow_maps_cascades(ID3D11DeviceContext4& dc) noexcept;

   void draw_to_target_map(ID3D11DeviceContext4& dc, const Draw_args& args) noexcept;

   void fill_visibility_lists(const glm::mat4& projection_matrix) noexcept;

   auto add_transform_cb(ID3D11DeviceContext4& dc, const Transform_cb& cb) noexcept
      -> UINT;

   void resize_transform_cb_buffer(ID3D11DeviceContext4& dc,
                                   std::size_t needed_space) noexcept;

   auto add_skin(ID3D11DeviceContext4& dc,
                 const std::array<std::array<glm::vec4, 3>, 15>& skin) noexcept -> UINT;

   void resize_skins_buffer(ID3D11DeviceContext4& dc, std::size_t needed_space) noexcept;
};

}