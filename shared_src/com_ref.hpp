#pragma once

#include "com_ptr.hpp"

namespace sp {

template<typename Class>
class Com_ref {
public:
   static_assert(detail::is_com_class_v<Class>,
                 "Class does not implement AddRef and Release.");

   explicit Com_ref(const Com_ptr<Class>& from) noexcept : _pointer{from}
   {
      Expects(_pointer != nullptr);
   }

   explicit Com_ref(Com_ptr<Class>&& from) noexcept : _pointer{std::move(from)}
   {
      Expects(_pointer != nullptr);
   }

   Com_ref(Class& from) noexcept : _pointer{&from}
   {
      from.AddRef();
   }

   Com_ref(const Com_ref& other) noexcept = default;

   Com_ref() = delete;
   Com_ref& operator=(const Com_ref& other) noexcept = delete;
   Com_ref& operator=(Com_ref&& other) noexcept = delete;

   Class* get() const noexcept
   {
      return _pointer.get();
   }

   Class& operator*() const noexcept
   {
      return *get();
   }

   Class* operator->() const noexcept
   {
      return get();
   }

   operator Class&() const noexcept
   {
      return *get();
   }

   explicit operator Com_ptr<Class>()
   {
      _pointer->AddRef();

      return Com_ptr{get()};
   }

   Class* operator&() const noexcept
   {
      return get();
   }

private:
   const Com_ptr<Class> _pointer;
};
}
