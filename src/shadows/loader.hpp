#pragma once

namespace sp::shadows {

void queue_load_lvl(const char* file_name) noexcept;

void wait_all_loaded() noexcept;

}