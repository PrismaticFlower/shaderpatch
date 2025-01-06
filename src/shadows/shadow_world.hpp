#pragma once

#include "input.hpp"

struct ID3D11Device2;
struct ID3D11DeviceContext2;

namespace sp::shadows {

struct Shadow_world_interface {
   static void initialize(ID3D11Device2& device);

   static void process_mesh_copy_queue(ID3D11DeviceContext2& dc) noexcept;

   static void clear() noexcept;

   static void add_model(const Input_model& model) noexcept;

   static void add_game_model(const Input_game_model& game_model) noexcept;

   static void add_object(const Input_object_class& object_class) noexcept;

   static void add_object_instance(const Input_instance& instance) noexcept;

   static void show_imgui(ID3D11DeviceContext2& dc) noexcept;
};

extern Shadow_world_interface shadow_world;

}