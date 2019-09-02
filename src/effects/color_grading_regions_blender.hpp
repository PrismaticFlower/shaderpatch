#pragma once

#include "color_grading_eval_params.hpp"
#include "color_grading_regions_io.hpp"
#include "postprocess_params.hpp"
#include "small_function.hpp"

#include <filesystem>
#include <memory>
#include <variant>

#include <Windows.h>

namespace sp::effects {

class Color_grading_regions_blender {
public:
   void global_params(const Color_grading_params& params) noexcept;

   auto global_params() const noexcept -> Color_grading_params;

   void regions(const Color_grading_regions& regions) noexcept;

   auto blend(const glm::vec3 camera_position) noexcept -> Color_grading_params;

   void show_imgui(const HWND game_window,
                   Small_function<Color_grading_params(Color_grading_params) noexcept>
                      show_cg_params_imgui) noexcept;

private:
   void init_region_params(const Color_grading_regions& regions) noexcept;

   void init_regions(const Color_grading_regions& regions) noexcept;

   auto get_region_params(const std::string_view config_name) noexcept -> std::size_t;

   struct Region {
      struct Box {
         glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
         glm::vec3 centre = {0.0f, 0.0f, 0.0f};
         glm::vec3 length = {0.0f, 0.0f, 0.0f};
         float inv_fade_length = 0.0f;

         auto weight(const glm::vec3 camera_position) const noexcept -> float
         {
            const auto point = (camera_position - centre) * rotation + centre;
            const auto vec = glm::max(glm::abs(point - centre) - length / 2.0f, 0.0f);
            const auto dst = glm::sqrt(glm::dot(vec, vec));

            return 1.0f - glm::clamp(dst * inv_fade_length, 0.0f, 1.0f);
         }
      };

      struct Sphere {
         glm::vec3 centre = {0.0f, 0.0f, 0.0f};
         float radius = 0.0f;
         float inv_fade_length = 0.0f;

         auto weight(const glm::vec3 camera_position) const noexcept -> float
         {
            const auto dst =
               glm::max(glm::distance(camera_position, centre) - radius, 0.0f);

            return 1.0f - glm::clamp(dst * inv_fade_length, 0.0f, 1.0f);
         }
      };

      struct Cylinder {
         glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
         glm::vec3 centre = {0.0f, 0.0f, 0.0f};
         float radius = 0.0f;
         float length = 0.0f;
         float inv_fade_length = 0.0f;

         auto weight(const glm::vec3 camera_position) const noexcept -> float
         {
            const auto point = (camera_position - centre) * rotation + centre;
            const auto edge_dst =
               glm::max(glm::distance(glm::vec2{point.x, point.z},
                                      glm::vec2{centre.x, centre.z}) -
                           radius,
                        0.0f);
            const auto cap_dst =
               glm::max(glm::distance(point.y, centre.y) - length, 0.0f);
            const auto dst = glm::max(edge_dst, cap_dst);

            return 1.0f - glm::clamp(dst * inv_fade_length, 0.0f, 1.0f);
         }
      };

      std::variant<Box, Sphere, Cylinder> primitive;
      const std::size_t params_index;

      Region(const Color_grading_region_desc& desc,
             const std::size_t params_index) noexcept;

      auto weight(const glm::vec3 camera_position) const noexcept -> float
      {
         return std::visit(
            [camera_position](const auto& prim) noexcept->float {
               return prim.weight(camera_position);
            },
            primitive);
      }
   };

   struct Contribution {
      float weight = 0.0f;
      const Color_grading_params& params;
   };

   std::vector<Contribution> contributions;

   Color_grading_params _global_params{};

   std::vector<Region> _regions;
   std::vector<Color_grading_params> _region_params;

   std::vector<std::string> _region_names;
   std::vector<std::string> _region_params_names;

   struct Config_bool {
      bool b = false;

      operator bool() const noexcept
      {
         return b;
      }

      operator bool*() noexcept
      {
         return &b;
      }

      operator const bool*() const noexcept
      {
         return &b;
      }
   };

   static_assert(sizeof(Config_bool) == sizeof(bool));

   std::vector<Config_bool> _imgui_editor_state;
};

}