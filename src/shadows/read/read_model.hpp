#pragma once

#include "../input.hpp"

#include "ucfb_file_reader.hpp"

namespace sp::shadows {

auto read_model(ucfb::File_reader_child& modl) -> Input_model;

}