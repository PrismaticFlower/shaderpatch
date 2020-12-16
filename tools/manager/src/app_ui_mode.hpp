#pragma once

#include "framework.hpp"

class app_ui_mode {
public:
   virtual ~app_ui_mode() = default;

   virtual void update(MSG& msg) noexcept = 0;
};
