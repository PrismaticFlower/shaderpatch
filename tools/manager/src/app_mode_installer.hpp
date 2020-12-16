#pragma once

#include "framework.hpp"

#include "app_ui_mode.hpp"

auto make_app_mode_installer() -> std::unique_ptr<app_ui_mode>;
