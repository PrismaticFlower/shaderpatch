#include "shadow_world.hpp"

#include "internal/name_table.hpp"

#include "../logger.hpp"

#include "com_ptr.hpp"

#include <d3d11_2.h>

#include <shared_mutex>

namespace sp::shadows {

namespace {

struct Shadow_world {
   Shadow_world(ID3D11Device2& device) : _device{copy_raw_com_ptr(device)} {}

   void clear() noexcept
   {
      std::scoped_lock lock{_mutex};

      _name_table.clear();
   }

   void add_model(Input_model&& model) noexcept
   {
      std::scoped_lock lock{_mutex};

      log_fmt(Log_level::info, "Read model '{}' (vert min: X {} Y {} Z {} vert max: X {} Y {} Z {} segments: {})",
              model.name, model.min_vertex.x, model.min_vertex.y,
              model.min_vertex.z, model.max_vertex.x, model.max_vertex.y,
              model.max_vertex.z, model.segments.size());

      const std::uint32_t name_hash = _name_table.add(model.name);

      (void)name_hash;
   }

   void add_game_model(Input_game_model&& game_model) noexcept
   {
      std::scoped_lock lock{_mutex};

      (void)game_model;
   }

   void add_object(Input_object_class&& object_class) noexcept
   {
      std::scoped_lock lock{_mutex};

      (void)object_class;
   }

   void add_object_instance(Input_instance&& instance) noexcept
   {
      std::scoped_lock lock{_mutex};

      (void)instance;
   }

private:
   std::shared_mutex _mutex;

   Com_ptr<ID3D11Device2> _device;

   Name_table _name_table;
};

std::atomic<Shadow_world*> shadow_world_ptr = nullptr;

}

Shadow_world_interface shadow_world;

void Shadow_world_interface::initialize(ID3D11Device2& device)
{
   if (shadow_world_ptr.load() != nullptr) {
      log(Log_level::warning, "Attempt to initialize Shadow World twice.");

      return;
   }

   static Shadow_world shadow_world_impl{device};

   shadow_world_ptr.store(&shadow_world_impl);
}

void Shadow_world_interface::clear() noexcept
{
   Shadow_world* self = shadow_world_ptr.load(std::memory_order_relaxed);

   if (!self) return;

   self->clear();
}

void Shadow_world_interface::add_model(Input_model&& model) noexcept
{
   Shadow_world* self = shadow_world_ptr.load(std::memory_order_relaxed);

   if (!self) return;

   self->add_model(std::move(model));
}

void Shadow_world_interface::add_game_model(Input_game_model&& game_model) noexcept
{
   Shadow_world* self = shadow_world_ptr.load(std::memory_order_relaxed);

   if (!self) return;

   self->add_game_model(std::move(game_model));
}

void Shadow_world_interface::add_object(Input_object_class&& object_class) noexcept
{
   Shadow_world* self = shadow_world_ptr.load(std::memory_order_relaxed);

   if (!self) return;

   self->add_object(std::move(object_class));
}

void Shadow_world_interface::add_object_instance(Input_instance&& instance) noexcept
{
   Shadow_world* self = shadow_world_ptr.load(std::memory_order_relaxed);

   if (!self) return;

   self->add_object_instance(std::move(instance));
}

}