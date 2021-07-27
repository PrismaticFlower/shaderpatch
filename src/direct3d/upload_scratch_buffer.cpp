
#include "upload_scratch_buffer.hpp"
#include "../logger.hpp"
#include "utility.hpp"

#include <exception>

using namespace std::literals;

namespace sp::d3d9 {

Upload_scratch_buffer upload_scratch_buffer;

Upload_scratch_buffer::Upload_scratch_buffer(const std::size_t max_persist_size,
                                             const std::size_t starting_size) noexcept
   : _max_persist_size{max_persist_size}
{
   resize(starting_size);
}

std::byte* Upload_scratch_buffer::lock(const std::size_t required_size) noexcept
{
   if (_locked.exchange(true))
      log_and_terminate("Attempt to lock scratch buffer already in use!"sv);

   ensure_min_size(required_size);

   return reinterpret_cast<std::byte*>(_memory.get());
}

void Upload_scratch_buffer::unlock() noexcept
{
   if (!_locked.exchange(false))
      log_and_terminate("Attempt to unlock scratch buffer that is not in use!"sv);

   ensure_within_size(_max_persist_size);
}

void Upload_scratch_buffer::ensure_min_size(const std::size_t required_size) noexcept
{
   if (_size >= required_size) return;

   resize(required_size);
}

void Upload_scratch_buffer::ensure_within_size(const std::size_t max_size) noexcept
{
   if (_size <= max_size) return;

   resize(max_size);
}

void Upload_scratch_buffer::resize(const std::size_t new_size) noexcept
{
   if (_size == new_size) return;

   if (new_size == 0) {
      _size = 0;
      _memory = nullptr;

      return;
   }

   const auto block_count = next_multiple_of<sizeof(Block)>(new_size) / sizeof(Block);

   _memory = std::unique_ptr<Block[]>{new (std::nothrow) Block[block_count]};
   _size = new_size;

   if (!_memory)
      log_and_terminate("Failed to allocate memory for upload scratch buffer!"sv);
}

}
