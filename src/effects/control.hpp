#pragma once

#include <gsl/gsl>

namespace sp::effects {

class Control {
public:
   bool enabled(bool enable) noexcept
   {
      return _enabled = enable;
   }

   bool enabled() const noexcept
   {
      return _enabled;
   }

   bool active(bool active) noexcept
   {
      return _active = active;
   }

   bool active() const noexcept
   {
      return _active;
   }

   void show_imgui() noexcept;

private:
   bool _enabled = false;
   bool _active = false;
};

}
