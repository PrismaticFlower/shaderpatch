#pragma once

#include "input.hpp"

struct ID3D11Device2;

namespace sp::shadows {

struct Shadow_world_interface {
   static void initialize(ID3D11Device2& device);

   static void clear() noexcept;

   static void add_model(Input_model&& model) noexcept;

   static void add_game_model(Input_game_model&& game_model) noexcept;

   static void add_object(Input_object_class&& object_class) noexcept;

   static void add_object_instance(Input_instance&& instance) noexcept;
};

extern Shadow_world_interface shadow_world;

}