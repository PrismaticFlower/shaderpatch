#pragma once

#include <sstream>
namespace sp {

template<typename Exception, typename... Args>
[[nodiscard]] auto compose_exception(Args&&... args)
{
   std::ostringstream msg;

   const auto append = [&](auto&& arg) {
      msg << std::forward<decltype(arg)>(arg);

      return false;
   };

   [[maybe_unused]] auto expender = {false, append(args)...};

   return Exception{msg.str()};
}
}
