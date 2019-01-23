#pragma once

#include <functional>
#include <type_traits>
#include <utility>

namespace sp {

template<typename>
class Small_function;

template<typename Return, typename... Args>
class Small_function<Return(Args...) noexcept> {
public:
   Small_function() = default;

   Small_function(Small_function&& from) noexcept : Small_function{}
   {
      swap(from);
   }

   Small_function& operator=(Small_function&& from) noexcept
   {
      swap(from);

      return *this;
   }

   template<typename Invocable>
   Small_function(Invocable&& invocable) noexcept
   {
      static_assert(std::is_nothrow_invocable_r_v<Return, Invocable, Args...>,
                    "invocable's arguments or return type do not match");

      create(std::forward<Invocable>(invocable));
   }

   template<typename Invocable>
   Small_function& operator=(Invocable&& invocable) noexcept
   {
      static_assert(std::is_nothrow_invocable_r_v<Return, Invocable, Args...>,
                    "invocable's arguments or return type do not match");

      cleanup();

      create(std::forward<Invocable>(invocable));

      return *this;
   }

   Small_function& operator=(std::nullptr_t) noexcept
   {
      cleanup();

      return *this;
   }

   ~Small_function()
   {
      cleanup();
   }

   Small_function(const Small_function&) = delete;
   Small_function& operator=(const Small_function&) = delete;

   auto operator()(Args... args) const noexcept -> Return
   {
      _invoke(&_invocable_storage, std::forward<Args>(args)...);
   }

   explicit operator bool() const noexcept
   {
      return _invoke != nullptr;
   }

   void swap(Small_function& other) noexcept
   {
      using std::swap;

      swap(this->_invoke, other._invoke);
      swap(this->_destroy, other._destroy);
      swap(this->_invocable_storage, other._invocable_storage);
   }

private:
   template<typename Invocable>
   void create(Invocable&& invocable) noexcept
   {
      using Invocable_type = std::remove_reference_t<std::remove_cv_t<Invocable>>;

      static_assert(sizeof(Invocable_type) <= sizeof(_invocable_storage),
                    "Invocable_type is too large!");

      new (&_invocable_storage) Invocable_type{std::forward<Invocable>(invocable)};

      _invoke = [](const void* invocable_storage, Args... args) noexcept
      {
         const auto& invocable =
            *static_cast<const Invocable_type*>(invocable_storage);

         return std::invoke(invocable, std::forward<Args>(args)...);
      };

      _destroy = [](void* invocable_storage) noexcept
      {
         auto& invocable = *static_cast<Invocable_type*>(invocable_storage);

         if constexpr (!std::is_trivially_destructible_v<Invocable_type>) {
            invocable.~Invocable_type();
         }
      };
   }

   void cleanup() noexcept
   {
      if (_invoke) _destroy(&_invocable_storage);

      _invoke = nullptr;
      _destroy = nullptr;
   }

   using Invoke_fn = std::add_pointer_t<Return(const void*, Args...) noexcept>;
   using Destory_fn = std::add_pointer_t<void(void*) noexcept>;

   Invoke_fn _invoke = nullptr;
   Destory_fn _destroy = nullptr;
   std::aligned_storage_t<24, 4> _invocable_storage;
};

}
