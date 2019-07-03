#pragma once

#include <ctime>
#include <execution>
#include <filesystem>
#include <iomanip>
#include <sstream>

#include "../logger.hpp"
#include "swapchain.hpp"
#include "utility.hpp"

#include <DirectXTex.h>
#include <d3d11_2.h>
#include <wincodec.h>

#include <gsl/gsl>

#pragma warning(disable : 4996) // std::localtime use

namespace sp::core {

using namespace std::literals;

namespace {

auto date_time_string() -> std::string
{
   const auto time = std::time(nullptr);
   const auto local_time = *std::localtime(&time);

   std::ostringstream stream;
   stream << std::put_time(&local_time, "%F %H_%M_%S");

   return stream.str();
}

auto pick_save_file(const std::filesystem::path& save_folder) noexcept
   -> std::filesystem::path
{
   const auto date_time = date_time_string();

   for (std::uint32_t i = 0; i < std::numeric_limits<std::uint32_t>::max(); ++i) {
      std::filesystem::path path{save_folder};
      path += date_time;

      if (i != 0) path += "#"s + std::to_string(i);

      path += ".png"s;

      if (!std::filesystem::exists(path)) return path;
   }

   log_and_terminate("Failed to find free file for screenshot!");
}

void make_opaque(const DirectX::Image image) noexcept
{
   Expects(image.format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB ||
           image.format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB);

   std::for_each_n(std::execution::par_unseq, Index_iterator{}, image.height, [&](const auto y) {
      const gsl::span<std::uint8_t> row{image.pixels + y * image.rowPitch,
                                        static_cast<std::ptrdiff_t>(image.rowPitch)};

      for (auto x = 0; x < image.width; ++x) {
         row[x * 4 + 3] = 0xff;
      }
   });
}

void save_screenshot(const std::filesystem::path& save_file, const Swapchain& swapchain,
                     const D3D11_MAPPED_SUBRESOURCE mapped_cpu_texture) noexcept
{
   DirectX::Image image;
   image.format = DirectX::MakeSRGB(swapchain.format);
   image.width = swapchain.width();
   image.height = swapchain.height();
   image.rowPitch = mapped_cpu_texture.RowPitch;
   image.slicePitch = mapped_cpu_texture.DepthPitch;
   image.pixels = static_cast<std::uint8_t*>(mapped_cpu_texture.pData);

   make_opaque(image);

   if (FAILED(DirectX::SaveToWICFile(image, 0, GUID_ContainerFormatPng,
                                     save_file.c_str())))
      log(Log_level::error, "Failed to save screenshot ", save_file, ".");
   else
      log(Log_level::info, "Saved screenshot ", save_file, ".");
}
}

void screenshot(ID3D11Device2& device, ID3D11DeviceContext2& dc,
                const Swapchain& swapchain,
                const std::filesystem::path& save_folder) noexcept
{
   Expects(!std::filesystem::exists(save_folder) ||
           std::filesystem::is_directory(save_folder));

   std::filesystem::create_directory(save_folder);

   const auto cpu_texture = [&] {
      Com_ptr<ID3D11Texture2D> texture;

      const CD3D11_TEXTURE2D_DESC desc{swapchain.format,
                                       swapchain.width(),
                                       swapchain.height(),
                                       1,
                                       1,
                                       0,
                                       D3D11_USAGE_STAGING,
                                       D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE};

      device.CreateTexture2D(&desc, nullptr, texture.clear_and_assign());

      return texture;
   }();

   dc.CopyResource(cpu_texture.get(), swapchain.texture());

   D3D11_MAPPED_SUBRESOURCE mapped_texture;
   dc.Map(cpu_texture.get(), 0, D3D11_MAP_READ_WRITE, 0, &mapped_texture);

   save_screenshot(pick_save_file(save_folder), swapchain, mapped_texture);

   dc.Unmap(cpu_texture.get(), 0);
}
}
