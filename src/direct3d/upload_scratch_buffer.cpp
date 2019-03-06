
#include "upload_scratch_buffer.hpp"
#include "../logger.hpp"
#include "utility.hpp"

#include <exception>

#include <Windows.h>

namespace sp::d3d9 {

Upload_scratch_buffer upload_scratch_buffer;

Upload_scratch_buffer::Upload_scratch_buffer(const std::size_t max_persist_size,
                                             const std::size_t starting_size) noexcept
   : _max_persist_size{max_persist_size}, _page_size{[] {
        SYSTEM_INFO info;
        GetSystemInfo(&info);

        return info.dwPageSize;
     }()}
{
   resize(starting_size);
}

Upload_scratch_buffer::~Upload_scratch_buffer()
{
   if (_data) VirtualFree(_data, _size, MEM_RELEASE);
}

std::byte* Upload_scratch_buffer::lock(const std::size_t required_size) noexcept
{
   if (_locked.exchange(true))
      log_and_terminate("Attempt to lock scratch buffer already in use!"sv);

   ensure_min_size(required_size);

   return _data;
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
      _data = nullptr;
      _size = 0;

      return;
   }

   if (_data) VirtualFree(_data, _size, MEM_RELEASE);

   _accessible_size = next_multiple_of(_page_size, new_size);
   _size = _accessible_size + _page_size;
   _data = static_cast<std::byte*>(
      VirtualAlloc(nullptr, _size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));

   if (!_data)
      log_and_terminate("Failed to allocate memory for upload scratch buffer!"sv);

   if (!VirtualFree(_data + _accessible_size, _page_size, MEM_DECOMMIT))
      log_and_terminate("Failed to install protection page for upload scratch buffer!"sv);

   _size = new_size;
}

}
