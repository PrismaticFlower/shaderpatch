
#include "profiler.hpp"

#include <algorithm>
#include <numeric>

#include <gsl/gsl>

#include "../imgui/imgui.h"

namespace sp::effects {

namespace {
struct Profile_timings {
   std::chrono::high_resolution_clock::time_point last_start_time;
   float average_duration;
   float max_duration;
   float min_duration;
   std::string_view name;
};
}

Profiler::Profiler(Com_ptr<ID3D11Device1> device) noexcept
   : _device{std::move(device)}
{
}

auto Profiler::begin_section(ID3D11DeviceContext1& dc,
                             const std::string_view name) noexcept -> std::size_t
{
   if (!enabled) return npos;

   const std::size_t index = [&]() {
      if (const auto index = find_section(name); index == npos) {
         _sections.emplace_back(name, *_device);

         return _sections.size() - 1;
      }
      else {
         return index;
      }
   }();

   auto& section = _sections[index].second;

   if (std::exchange(section.queried, true)) std::terminate();

   section.last_start_time = std::chrono::high_resolution_clock::now();

   dc.Begin(section.disjoint_queries[_current_frame].get());
   dc.End(section.time_stamp_begin_queries[_current_frame].get());

   return index;
}

void Profiler::end_section(ID3D11DeviceContext1& dc, const std::size_t index) noexcept
{
   Expects(index < _sections.size() || index == npos);

   if (!enabled || index == npos) return;

   Section_data& section = _sections[index].second;

   if (!std::exchange(section.queried, false)) std::terminate();

   dc.End(section.time_stamp_end_queries[_current_frame].get());
   dc.End(section.disjoint_queries[_current_frame].get());

   section.active = true;
}

void Profiler::end_frame(ID3D11DeviceContext1& dc) noexcept
{
   _current_frame = (_current_frame + 1) % query_frame_latency;

   if (!enabled) return;

   std::vector<Profile_timings> timings_list;
   timings_list.reserve(_sections.size());

   for (auto& name_section : _sections) {
      Section_data& section = name_section.second;

      if (!std::exchange(section.active, false)) continue;

      UINT64 begin{};
      while (dc.GetData(section.time_stamp_begin_queries[_current_frame].get(),
                        &begin, sizeof(begin), 0) == S_FALSE)
         ;

      UINT64 end{};
      while (dc.GetData(section.time_stamp_end_queries[_current_frame].get(),
                        &end, sizeof(end), 0) == S_FALSE)
         ;

      D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint{};
      while (dc.GetData(section.disjoint_queries[_current_frame].get(),
                        &disjoint, sizeof(disjoint), 0) == S_FALSE)
         ;

      if (!disjoint.Disjoint) {
         const auto delta = end - begin;
         const auto duration = delta / static_cast<float>(disjoint.Frequency) * 1000.0f;

         section.duration_samples[section.current_duration_sample] = duration;
         section.max_duration = std::max(section.max_duration, duration);
         section.min_duration = std::min(section.min_duration, duration);
         section.current_duration_sample = (section.current_duration_sample + 1) %
                                           section.duration_samples.size();
      }

      Profile_timings timings;
      timings.last_start_time = section.last_start_time;
      timings.average_duration = std::reduce(section.duration_samples.cbegin(),
                                             section.duration_samples.cend()) /
                                 section.duration_samples.size();
      timings.max_duration = section.max_duration;
      timings.min_duration = section.min_duration;
      timings.name = name_section.first;

      timings_list.emplace_back(timings);
   }

   std::sort(timings_list.begin(), timings_list.end(),
             [](const Profile_timings& left, const Profile_timings& right) {
                return left.last_start_time < right.last_start_time;
             });

   const ImVec2 window_pos{10.0f, 10.0f};
   const ImVec2 window_pos_pivot{0.0f, 0.0f};

   ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
   ImGui::SetNextWindowBgAlpha(0.3f);

   ImGui::Begin("GPU Profiling", nullptr,
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar |
                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize |
                   ImGuiWindowFlags_NoSavedSettings |
                   ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);

   ImGui::Text("Effects GPU Timings");

   ImGui::Separator();

   for (const auto& timings : timings_list) {
      ImGui::Text("%s avg: %f max: %f min %f", timings.name.data(),
                  timings.average_duration, timings.max_duration,
                  timings.min_duration);
   }

   ImGui::End();
}

auto Profiler::find_section(const std::string_view name) const noexcept -> std::size_t
{
   for (std::size_t i = 0; i < _sections.size(); ++i) {
      if (_sections[i].first == name) return i;
   }

   return npos;
}

Profiler::Section_data::Section_data(ID3D11Device1& device) noexcept
{
   for (auto& query : disjoint_queries) {
      const CD3D11_QUERY_DESC desc{D3D11_QUERY_TIMESTAMP_DISJOINT};
      device.CreateQuery(&desc, query.clear_and_assign());
   }

   for (auto& query : time_stamp_begin_queries) {
      const CD3D11_QUERY_DESC desc{D3D11_QUERY_TIMESTAMP};
      device.CreateQuery(&desc, query.clear_and_assign());
   }

   for (auto& query : time_stamp_end_queries) {
      const CD3D11_QUERY_DESC desc{D3D11_QUERY_TIMESTAMP};
      device.CreateQuery(&desc, query.clear_and_assign());
   }
}

Profile::Profile(Profiler& profiler, ID3D11DeviceContext1& dc,
                 const std::string_view name) noexcept
   : _profiler{profiler}, _dc{dc}, _index{profiler.begin_section(dc, name)}
{
}

Profile::~Profile()
{
   _profiler.end_section(_dc, _index);
}

}
