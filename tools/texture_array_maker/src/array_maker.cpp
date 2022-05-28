
#include "array_maker.hpp"
#include "com_ptr.hpp"
#include "file_dialogs.hpp"
#include "string_utilities.hpp"

#include "framework/imgui.h"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include <DirectXTex.h>
#include <DirectXTexEXR.h>

using namespace sp;

namespace {

HWND window_handle;
Com_ptr<ID3D11Device> d3d11_device;
std::vector<std::filesystem::path> input_paths;
std::vector<Com_ptr<ID3D11ShaderResourceView>> preview_images;
std::vector<DirectX::ScratchImage> folded_cubemaps;

std::string error_message;

auto load_image(const std::filesystem::path& path) -> DirectX::ScratchImage
{
   DirectX::ScratchImage image;
   HRESULT result;

   if (const auto ext = path.extension().string(); ext == ".dds"_svci) {
      throw std::runtime_error{"Input can not be DDS file."};
   }
   else if (ext == ".hdr"_svci) {
      result = DirectX::LoadFromHDRFile(path.c_str(), nullptr, image);
   }
   else if (ext == ".tga"_svci) {
      result = DirectX::LoadFromTGAFile(path.c_str(), nullptr, image);
   }
   else if (ext == ".exr"_svci) {
      result = DirectX::LoadFromEXRFile(path.c_str(), nullptr, image);
   }
   else {
      result = DirectX::LoadFromWICFile(path.c_str(), DirectX::WIC_FLAGS_NONE,
                                        nullptr, image);
   }

   if (FAILED(result)) {
      throw std::runtime_error{"Failed to load image file."};
   }

   image.OverrideFormat(DirectX::MakeSRGB(image.GetMetadata().format));

   auto& meta = image.GetMetadata();

   const auto face_width = (meta.width / 4);
   const auto face_height = (meta.height / 3);

   if (face_width * 4 != meta.width || face_height * 3 != meta.height) {
      throw std::runtime_error{"Invalid texture dimensions for cubemap!"};
   }

   return image;
}

auto fold_cubemap(const DirectX::ScratchImage& image) -> DirectX::ScratchImage
{
   auto& meta = image.GetMetadata();

   const auto width = (meta.width / 4);
   const auto height = (meta.height / 3);

   DirectX::ScratchImage cubemap;
   cubemap.InitializeCube(meta.format, width, height, 1, 1);

   // +X
   DirectX::CopyRectangle(*image.GetImage(0, 0, 0), {width * 2, height, width, height},
                          *cubemap.GetImage(0, 0, 0),
                          DirectX::TEX_FILTER_FORCE_NON_WIC, 0, 0);

   // -X
   DirectX::CopyRectangle(*image.GetImage(0, 0, 0), {0, height, width, height},
                          *cubemap.GetImage(0, 1, 0),
                          DirectX::TEX_FILTER_FORCE_NON_WIC, 0, 0);

   // +Y
   DirectX::CopyRectangle(*image.GetImage(0, 0, 0), {width, 0, width, height},
                          *cubemap.GetImage(0, 2, 0),
                          DirectX::TEX_FILTER_FORCE_NON_WIC, 0, 0);

   // -Y
   DirectX::CopyRectangle(*image.GetImage(0, 0, 0), {width, height * 2, width, height},
                          *cubemap.GetImage(0, 3, 0),
                          DirectX::TEX_FILTER_FORCE_NON_WIC, 0, 0);

   // +Z
   DirectX::CopyRectangle(*image.GetImage(0, 0, 0), {width, height, width, height},
                          *cubemap.GetImage(0, 4, 0),
                          DirectX::TEX_FILTER_FORCE_NON_WIC, 0, 0);

   // +Z
   DirectX::CopyRectangle(*image.GetImage(0, 0, 0), {width * 3, height, width, height},
                          *cubemap.GetImage(0, 5, 0),
                          DirectX::TEX_FILTER_FORCE_NON_WIC, 0, 0);

   return cubemap;
}

auto mipmap_preview(const DirectX::ScratchImage& image) -> DirectX::ScratchImage
{
   DirectX::ScratchImage mipped_image;

   if (FAILED(DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(),
                                       image.GetMetadata(),
                                       DirectX::TEX_FILTER_DEFAULT | DirectX::TEX_FILTER_FORCE_NON_WIC,
                                       0, mipped_image))) {
      throw std::runtime_error{"Unable to generate mip maps"};
   }

   return mipped_image;
}

auto make_preview(const DirectX::ScratchImage& image) -> Com_ptr<ID3D11ShaderResourceView>
{
   Com_ptr<ID3D11ShaderResourceView> srv;

   if (FAILED(DirectX::CreateShaderResourceView(d3d11_device.get(), image.GetImages(),
                                                image.GetImageCount(), image.GetMetadata(),
                                                srv.clear_and_assign()))) {
      throw std::runtime_error{"Unable to create preview image! This means "
                               "you're using too high a resolution image."};
   }

   return srv;
}

void validate_image_compatible(const DirectX::ScratchImage& image)
{
   if (folded_cubemaps.empty()) return;

   const auto& left = folded_cubemaps[0].GetMetadata();
   const auto& right = image.GetMetadata();

   if (left.width != right.width || left.height != right.height ||
       left.depth != right.depth || left.arraySize != right.arraySize ||
       left.format != right.format || left.dimension != right.dimension) {
      throw std::runtime_error{"Images must be same size and format!"};
   }
}

void add_image(const std::wstring& path)
{
   try {
      DirectX::ScratchImage image = load_image(path);
      DirectX::ScratchImage cubemap = fold_cubemap(image);

      validate_image_compatible(cubemap);

      image = mipmap_preview(image);

      Com_ptr<ID3D11ShaderResourceView> preview = make_preview(image);

      input_paths.emplace_back(path);
      preview_images.emplace_back(preview);
      folded_cubemaps.emplace_back(std::move(cubemap));
   }
   catch (std::exception& e) {
      error_message = e.what();
   }
}

void build_array(const std::filesystem::path& path)
{
   if (folded_cubemaps.empty()) return;

   DirectX::ScratchImage image;

   auto& meta = folded_cubemaps[0].GetMetadata();

   image.InitializeCube(meta.format, meta.width, meta.height,
                        folded_cubemaps.size(), 1);

   for (std::size_t i = 0; i < folded_cubemaps.size(); ++i) {
      for (std::size_t face = 0; face < 6; ++face) {
         DirectX::CopyRectangle(*folded_cubemaps[i].GetImage(0, face, 0),
                                {0, 0, meta.width, meta.height},
                                *image.GetImage(0, i * 6 + face, 0),
                                DirectX::TEX_FILTER_FORCE_NON_WIC, 0, 0);
      }
   }

   if (FAILED(DirectX::SaveToDDSFile(image.GetImages(), image.GetImageCount(),
                                     image.GetMetadata(),
                                     DirectX::DDS_FLAGS_NONE, path.c_str()))) {
      error_message = "Failed to save texture array!";
   }
}

}

void array_maker_init(ID3D11Device& device, HWND window)
{
   d3d11_device = copy_raw_com_ptr(device);
   window_handle = window;
}

void array_maker(const float display_scale)
{
   auto& io = ImGui::GetIO();
   auto* viewport = ImGui::GetMainViewport();

   ImGui::SetNextWindowPos(viewport->WorkPos);
   ImGui::SetNextWindowSize(viewport->WorkSize);
   ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);

   ImGui::Begin("##main_window", nullptr,
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

   ImGui::PopStyleVar();

   if (ImGui::Button("Build Array") && not input_paths.empty()) {
      if (auto path = win32::save_file_dialog({{L"DDS Texture", L"*.DDS"}}, window_handle,
                                              std::filesystem::current_path(),
                                              L"cube_array.dds");
          path) {
         build_array(*path);
      }
   }
   ImGui::SameLine();

   ImGui::Button("Add Texture...");

   ImGui::Text("TIP: You can drag image files onto this window from "
               "explorer to add them to the array.");

   ImGui::Separator();

   if (!error_message.empty()) {
      ImGui::Text("Error:");
      ImGui::Indent();
      ImGui::Text(error_message.c_str());
      ImGui::Unindent();
      ImGui::Separator();
   }

   for (std::size_t i = 0; i < input_paths.size(); ++i) {
      ImGui::PushID(static_cast<int>(i));

      ImGui::Image(preview_images[i].get(),
                   {64.0f * display_scale, 48.0f * display_scale});

      ImGui::SameLine();

      ImGui::SetCursorPos(
         {ImGui::GetCursorPos().x,
          ImGui::GetCursorPos().y +
             (48.0f * display_scale - ImGui::GetFont()->FontSize) / 2.0f});

      ImGui::Text(input_paths[i].string().c_str());

      ImGui::SameLine();

      ImGui::SetCursorPos({ImGui::GetCursorPos().x,
                           ImGui::GetCursorPos().y +
                              (48.0f * display_scale - 20.0f * display_scale) / 2.0f});

      if (ImGui::Button("X", {20.0f * display_scale, 20.0f * display_scale})) {
         input_paths.erase(input_paths.begin() + i);
         preview_images.erase(preview_images.begin() + i);
         folded_cubemaps.erase(folded_cubemaps.begin() + i);

         i--;
      }

      ImGui::PopID();
   }

   ImGui::End();
}

void array_maker_add_drop_files(HDROP files)
{
   UINT file_count = DragQueryFileW(files, 0xffffffff, nullptr, 0);

   input_paths.reserve(input_paths.size() + file_count);

   for (UINT i = 0; i < file_count; ++i) {
      UINT size = DragQueryFileW(files, i, nullptr, 0);

      std::wstring path;
      path.resize(size);

      DragQueryFileW(files, i, path.data(), size + 1);

      add_image(path);
   }

   DragFinish(files);
}
