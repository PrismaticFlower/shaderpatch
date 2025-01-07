#pragma once

#include "../input.hpp"

#include "ucfb_file_reader.hpp"

namespace sp::shadows {

auto read_game_model(ucfb::File_reader_child& gmod) -> Input_game_model;

}