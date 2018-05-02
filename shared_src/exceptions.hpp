#pragma once

#include <stdexcept>

namespace sp {

class Parse_error : public std::runtime_error {
public:
   using std::runtime_error::runtime_error;
};

}
