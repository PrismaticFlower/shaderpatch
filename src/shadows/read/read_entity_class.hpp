#pragma once

#include "../input.hpp"

#include "ucfb_file_reader.hpp"

namespace sp::shadows {

auto read_entity_class(ucfb::File_reader_child& entc) -> Input_entity_class;

}