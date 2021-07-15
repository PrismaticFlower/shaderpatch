#pragma once

#include <atomic>
#include <bit>
#include <memory>

namespace sp {

/// @brief A simple single producer single consumer queue. Intended for use with lightweight trivial datatypes.
/// @tparam T Data type to store, must be default constructible, copy constructible and trivially destructible.
///
/// Thread-safe for one thread calling write functions (`enqueue` and `wait_for_empty`)
/// and another calling read functions (`dequeue`).
template<typename T, std::size_t buffer_size>
class single_producer_single_consumer_queue {
public:
   static_assert(std::is_default_constructible_v<T>);
   static_assert(std::is_copy_constructible_v<T>);
   static_assert(std::is_trivially_destructible_v<T>);
   static_assert(std::has_single_bit(buffer_size));

   using value_type = T;

   /// @brief Add an item to the queue. If there is no space left in the queue then blocks until there is.
   /// @param value Thw item to add.
   void enqueue(const value_type& value) noexcept
   {
      auto desired_write = _write.load();

      if (const auto current_read = _read.load(); desired_write == current_read) {
         _read.wait(current_read);
      }

      new (&_buffer[desired_write]) value_type{value};

      _write.store((desired_write + 1) % buffer_size, std::memory_order_release);
      _write.notify_one();
   }

   /// @brief Removes an item from the queue. If there is nothing to remove then blocks until there is.
   /// @return The item from the queue.
   [[nodiscard]] auto dequeue() noexcept -> value_type
   {
      auto intended_read = (_read.load() + 1) % buffer_size;

      if (const auto unwanted_write = _write.load(); unwanted_write == intended_read) {
         _write.wait(unwanted_write);
      }

      auto data = _buffer[intended_read];

      _read.store(intended_read, std::memory_order_release);
      _read.notify_one();

      return data;
   }

   /// @brief Waits for the queue to be empty and only returns once it is.
   void wait_for_empty() const noexcept
   {
      const auto next_write = _write.load();

      while (true) {
         auto current_read = _read.load();

         if (((current_read + 1) % buffer_size) == next_write) break;

         _read.wait(current_read);
      }
   }

private:
   std::atomic_size_t _read = buffer_size - 1;
   std::atomic_size_t _write = 0;
   std::unique_ptr<value_type[]> _buffer =
      std::make_unique_for_overwrite<value_type[]>(buffer_size);
};

}
