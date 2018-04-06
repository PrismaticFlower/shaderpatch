#pragma once

#include <iosfwd>
#include <iostream>
#include <mutex>

namespace sp {

template<typename... Args>
inline void synced_print(Args&&... args)
{
   static std::mutex mutex;

   std::lock_guard<std::mutex> lock{mutex};

   const auto output = [](auto&& arg) {
      std::cout << std::forward<decltype(arg)>(arg);

      return false;
   };

   [[maybe_unused]] auto expender = {false, output(std::forward<Args>(args))...};

   std::cout << '\n';
}

template<typename... Args>
inline void synced_error_print(Args&&... args)
{
   static std::mutex mutex;

   std::lock_guard<std::mutex> lock{mutex};

   const auto output = [](auto&& arg) {
      std::cerr << std::forward<decltype(arg)>(arg);

      return false;
   };

   [[maybe_unused]] auto expender = {false, output(std::forward<Args>(args))...};

   std::cerr << '\n';
}
}
