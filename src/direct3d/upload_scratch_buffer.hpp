#pragma once

#include <atomic>
#include <cstddef>

#include <gsl/gsl>

namespace sp::d3d9 {

class Upload_scratch_buffer {
public:
   Upload_scratch_buffer(const std::size_t max_persist_size = 16777216u,
                         const std::size_t starting_size = 16777216u) noexcept;

   ~Upload_scratch_buffer();

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

   std::atomic_bool _locked = false;
   gsl::owner<std::byte*> _data = nullptr;
   std::size_t _accessible_size{};
   std::size_t _size{};

   const std::size_t _max_persist_size;
   const std::size_t _page_size;
};

extern Upload_scratch_buffer upload_scratch_buffer;

}
