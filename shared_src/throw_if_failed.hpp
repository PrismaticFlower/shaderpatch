#pragma once

#include <system_error>

namespace sp {

inline void throw_if_failed(long hr)
{
   if (hr < 0) {
      throw std::system_error{hr, std::system_category()};
   }
}

}
