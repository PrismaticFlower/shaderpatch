#pragma once

#include <functional>
#include <iosfwd>
#include <memory>
#include <type_traits>
#include <utility>

#include <gsl/gsl>

namespace sp {

namespace detail {

template<typename Type, typename = void>
struct Is_com_class : std::false_type {
};

template<typename Type>
struct Is_com_class<Type, std::void_t<decltype(std::declval<Type>().AddRef()),
                                      decltype(std::declval<Type>().Release())>>
   : std::true_type {
};

template<typename Type>
constexpr bool is_com_class_v = Is_com_class<Type>::value;
}

template<typename Class>
class Com_ptr {
public:
   static_assert(detail::is_com_class_v<Class>,
                 "Class does not implement AddRef and Release.");

   using element_type = Class;

   Com_ptr() = default;

   ~Com_ptr()
   {
      if (_pointer) _pointer->Release();
   }

   explicit Com_ptr(Class* from) noexcept
   {
      _pointer = from;
   }

   Com_ptr(std::nullptr_t){};

   Com_ptr(const Com_ptr& other) noexcept
   {
      if (other) {
         other->AddRef();

         _pointer = other.get();
      }
   }

   Com_ptr& operator=(const Com_ptr& other) noexcept
   {
      if (other) {
         other->AddRef();
      }

      reset(other.get());

      return *this;
   }

   Com_ptr(Com_ptr&& other) noexcept
   {
      _pointer = other.release();
   }

   Com_ptr& operator=(Com_ptr&& other) noexcept
   {
      reset(other.release());

      return *this;
   }

   template<typename Other, typename = std::enable_if_t<std::is_convertible_v<Other*, Class*>>>
   Com_ptr(const Com_ptr<Other>& other)
   {
      auto other_copy = other;

      _pointer = static_cast<Class*>(other_copy.release());
   }

   template<typename Other, typename = std::enable_if_t<std::is_convertible_v<Other*, Class*>>>
   Com_ptr& operator=(const Com_ptr<Other>& other) noexcept
   {
      auto other_copy = other;

      reset(static_cast<Class*>(other_copy.release()));

      return *this;
   }

   template<typename Other, typename = std::enable_if_t<std::is_convertible_v<Other*, Class*>>>
   Com_ptr(Com_ptr<Other>&& other)
   {
      _pointer = static_cast<Class*>(other.release());
   }

   template<typename Other, typename = std::enable_if_t<std::is_convertible_v<Other*, Class*>>>
   Com_ptr& operator=(Com_ptr<Other>&& other) noexcept
   {
      reset(static_cast<Class*>(other.release()));

      return *this;
   }

   template<typename Other, typename = std::enable_if_t<std::is_convertible_v<Other*, Class*>>>
   explicit Com_ptr(const std::shared_ptr<Other>& other)
   {
      _pointer = static_cast<Class*>(other.get());

      if (_pointer) _pointer->AddRef();
   }

   void reset(Class* with) noexcept
   {
      Com_ptr discarded{_pointer};

      _pointer = with;
   }

   void swap(Com_ptr& other) noexcept
   {
      std::swap(this->_pointer, other._pointer);
   }

   [[nodiscard]] Class* release() noexcept
   {
      return std::exchange(_pointer, nullptr);
   }

   Class* get() const noexcept
   {
      return _pointer;
   }

   Class& operator*() const noexcept
   {
      return *_pointer;
   }

   Class* operator->() const noexcept
   {
      return _pointer;
   }

   [[nodiscard]] Class** clear_and_assign() noexcept
   {
      Com_ptr discarded{};
      swap(discarded);

      return &_pointer;
   }

   [[nodiscard]] void** void_clear_and_assign() noexcept
   {
      Com_ptr discarded{};
      swap(discarded);

      static_assert(sizeof(void**) == sizeof(Class**));

      return reinterpret_cast<void**>(&_pointer);
   }

   [[nodiscard]] Class* unmanaged_copy() const noexcept
   {
      Expects(this->get() != nullptr);

      _pointer->AddRef();

      return _pointer;
   }

   explicit operator bool() const noexcept
   {
      return (_pointer != nullptr);
   }

private:
   Class* _pointer = nullptr;
};

template<typename Class>
inline void swap(Com_ptr<Class>& left, Com_ptr<Class>& right) noexcept
{
   left.swap(right);
}

template<typename Class>
inline bool operator==(const Com_ptr<Class>& left, const Com_ptr<Class>& right) noexcept
{
   return (left.get() == right.get());
}

template<typename Class>
inline bool operator!=(const Com_ptr<Class>& left, const Com_ptr<Class>& right) noexcept
{
   return (left.get() != right.get());
}

template<typename Class>
inline bool operator==(const Com_ptr<Class>& left, std::nullptr_t) noexcept
{
   return (left.get() == nullptr);
}

template<typename Class>
inline bool operator!=(const Com_ptr<Class>& left, std::nullptr_t) noexcept
{
   return (left.get() != nullptr);
}

template<typename Class>
inline bool operator==(std::nullptr_t, const Com_ptr<Class>& right) noexcept
{
   return (right == nullptr);
}

template<typename Class>
inline bool operator!=(std::nullptr_t, const Com_ptr<Class>& right) noexcept
{
   return (right != nullptr);
}

template<typename Class>
inline bool operator==(const Com_ptr<Class>& left, Class* const right) noexcept
{
   return (left.get() == right);
}

template<typename Class>
inline bool operator!=(const Com_ptr<Class>& left, Class* const right) noexcept
{
   return (left.get() != right);
}

template<typename Class>
inline bool operator==(Class* const left, const Com_ptr<Class>& right) noexcept
{
   return (right == left);
}

template<typename Class>
inline bool operator!=(Class* const left, const Com_ptr<Class>& right) noexcept
{
   return (right != left);
}

template<typename Class, typename Char, typename Traits>
inline auto operator<<(std::basic_ostream<Char, Traits>& ostream,
                       const Com_ptr<Class>& com_ptr)
   -> std::basic_ostream<Char, Traits>&
{
   return ostream << com_ptr.get();
}

template<typename Class>
inline auto make_shared_com_ptr(Com_ptr<Class> com_ptr) -> std::shared_ptr<Class>
{
   return {com_ptr.release(), [](Class* ptr) {
              if (ptr) ptr->Release();
           }};
}
}

template<typename Class>
struct std::hash<sp::Com_ptr<Class>> {
   auto operator()(const sp::Com_ptr<Class>& ptr) const noexcept
   {
      return std::hash<Class*>{}(ptr.get());
   }
};
