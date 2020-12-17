#pragma once

#include "framework.hpp"

enum class app_update_result { none, switch_to_configurator };

class app_ui_mode {
public:
   virtual ~app_ui_mode() = default;

   virtual auto update(MSG& msg) noexcept -> app_update_result = 0;
};
