#pragma once

#include <atomic>
#include <cstddef>
#include <memory>

#include <gsl/gsl>

namespace sp::d3d9 {

class Upload_scratch_buffer {
public:
   Upload_scratch_buffer(const std::size_t max_persist_size = 16777216u,
                         const std::size_t starting_size = 16777216u) noexcept;

   ~Upload_scratch_buffer() = default;

   Upload_scratch_buffer(const Upload_scratch_buffer&) = delete;
   Upload_scratch_buffer& operator=(const Upload_scratch_buffer&) = delete;

   Upload_scratch_buffer(Upload_scratch_buffer&&) = delete;
   Upload_scratch_buffer& operator=(Upload_scratch_buffer&&) = delete;

   std::byte* lock(const std::size_t required_size) noexcept;

   void unlock() noexcept;

private:
   void ensure_min_size(const std::size_t required_size) noexcept;

   void ensure_within_size(const std::size_t max_size) noexcept;

   void resize(const std::size_t new_size) noexcept;

   using Block = std::aligned_storage_t<65536, 16>;

   std::atomic_bool _locked = false;
   std::unique_ptr<Block[]> _memory = nullptr;
   std::size_t _size = 0;

   const std::size_t _max_persist_size;
};

extern Upload_scratch_buffer upload_scratch_buffer;

}
