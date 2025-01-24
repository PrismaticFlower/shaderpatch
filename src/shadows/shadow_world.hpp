#pragma once

#include "input.hpp"

#include <span>

#include <d3d11_2.h>

namespace sp::shader {

class Database;

}

namespace sp::shadows {

struct Shadow_draw_args {
   INT depth_bias = 0;
   float depth_bias_clamp = 0.0f;
   float slope_scaled_depth_bias = 0.0f;
};

struct Shadow_draw_view {
   D3D11_VIEWPORT viewport;
   ID3D11DepthStencilView* dsv = nullptr;
   glm::mat4 shadow_projection_matrix;
};

struct Shadow_world_interface {
   static void initialize(ID3D11Device2& device, shader::Database& shaders);

   static void process_mesh_copy_queue(ID3D11DeviceContext2& dc) noexcept;

   static void draw_shadow_views(ID3D11DeviceContext2& dc, const Shadow_draw_args& args,
                                 std::span<const Shadow_draw_view> views) noexcept;

   static void draw_shadow_world_preview(ID3D11DeviceContext2& dc,
                                         const glm::mat4& projection_matrix,
                                         const D3D11_VIEWPORT& viewport,
                                         ID3D11RenderTargetView* rtv,
                                         ID3D11DepthStencilView* dsv) noexcept;

   static void draw_shadow_world_textured_preview(ID3D11DeviceContext2& dc,
                                                  const glm::mat4& projection_matrix,
                                                  const D3D11_VIEWPORT& viewport,
                                                  ID3D11RenderTargetView* rtv,
                                                  ID3D11DepthStencilView* dsv) noexcept;

   static void draw_shadow_world_aabb_overlay(ID3D11DeviceContext2& dc,
                                              const glm::mat4& projection_matrix,
                                              const D3D11_VIEWPORT& viewport,
                                              ID3D11RenderTargetView* rtv,
                                              ID3D11DepthStencilView* dsv) noexcept;
   static void clear() noexcept;

   static void add_texture(const Input_texture& texture) noexcept;

   static void add_model(const Input_model& model) noexcept;

   static void add_game_model(const Input_game_model& game_model) noexcept;

   static void add_entity_class(const Input_entity_class& entity_class) noexcept;

   static void add_object_instance(const Input_object_instance& instance) noexcept;

   static void register_texture(ID3D11ShaderResourceView& srv,
                                const Texture_hash& data_hash) noexcept;

   static void unregister_texture(ID3D11ShaderResourceView& srv) noexcept;

   static void show_imgui(ID3D11DeviceContext2& dc) noexcept;
};

extern Shadow_world_interface shadow_world;

}