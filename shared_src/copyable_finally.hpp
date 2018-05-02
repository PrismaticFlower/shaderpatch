#pragma once

#include <functional>
#include <type_traits>
#include <utility>

namespace sp {

class Copyable_finally {
public:
   Copyable_finally() = delete;

   Copyable_finally(const Copyable_finally&) = default;
   Copyable_finally& operator=(const Copyable_finally&) = default;

   Copyable_finally(Copyable_finally&&) = default;
   Copyable_finally& operator=(Copyable_finally&&) = default;

   template<typename Invocable>
   Copyable_finally(Invocable&& invocable)
      : _invocable{std::forward<Invocable>(invocable)}
   {
   }

   ~Copyable_finally()
   {
      if (_invocable) _invocable();
   }

private:
   std::function<void()> _invocable;
};

template<typename Invocable>
auto copyable_finally(Invocable&& invocable)
{
   return Copyable_finally{std::forward<Invocable>(invocable)};
}

}
