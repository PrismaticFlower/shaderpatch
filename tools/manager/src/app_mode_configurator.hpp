#pragma once

#include "framework.hpp"

#include "app_ui_mode.hpp"

auto make_app_mode_configurator() -> std::unique_ptr<app_ui_mode>;
