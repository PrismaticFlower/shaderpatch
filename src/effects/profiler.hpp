#pragma once

#include "com_ptr.hpp"

#include <array>
#include <chrono>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include <d3d11_1.h>

namespace sp::effects {

class Profiler {
public:
   explicit Profiler(Com_ptr<ID3D11Device1> device) noexcept;
   ~Profiler() = default;

   Profiler(const Profiler&) = delete;
   Profiler& operator=(const Profiler&) = delete;
   Profiler(Profiler&&) = delete;
   Profiler& operator=(Profiler&&) = delete;

   auto begin_section(ID3D11DeviceContext1& dc, const std::string_view name) noexcept
      -> std::size_t;

   void end_section(ID3D11DeviceContext1& dc, const std::size_t index) noexcept;

   void end_frame(ID3D11DeviceContext1& dc) noexcept;

   bool enabled = false;

private:
   constexpr static std::size_t query_frame_latency = 5;
   constexpr static std::size_t npos = std::numeric_limits<std::size_t>::max();

   auto find_section(const std::string_view name) const noexcept -> std::size_t;

   struct Section_data {
      Section_data(ID3D11Device1& device) noexcept;

      std::array<Com_ptr<ID3D11Query>, query_frame_latency> disjoint_queries{};
      std::array<Com_ptr<ID3D11Query>, query_frame_latency> time_stamp_begin_queries{};
      std::array<Com_ptr<ID3D11Query>, query_frame_latency> time_stamp_end_queries{};

      std::array<float, 60> duration_samples{};
      std::size_t current_duration_sample = 0;
      std::chrono::high_resolution_clock::time_point last_start_time;
      float max_duration = 0.0f;
      float min_duration = std::numeric_limits<float>::max();

      bool queried = false;
      bool active = false;
   };

   const Com_ptr<ID3D11Device1> _device;

   std::size_t _current_frame = 0;

   std::vector<std::pair<std::string, Section_data>> _sections;
};

class Profile {
public:
   Profile(Profiler& profiler, ID3D11DeviceContext1& dc,
           const std::string_view name) noexcept;

   ~Profile();

   Profile(const Profile&) = delete;
   Profile& operator=(const Profile&) = delete;
   Profile(Profile&&) = delete;
   Profile& operator=(Profile&&) = delete;

private:
   Profiler& _profiler;
   ID3D11DeviceContext1& _dc;
   const std::size_t _index;
};

}
