#include "framework.hpp"

#include "xml_install_manifest.hpp"

using namespace std::literals;
using namespace winrt::Windows::Data::Xml::Dom;

namespace {

constexpr auto xml_manifest_directory = LR"(data\shaderpatch\)"sv;
constexpr auto xml_manifest_file = L"install_info.xml"sv;

}

auto load_xml_install_manifest(const std::filesystem::path& base_path)
   -> std::vector<std::filesystem::path>
{
   XmlDocument xml;

   try {
      using namespace winrt::Windows::Storage;

      auto full_path = base_path / xml_manifest_directory;
      full_path.make_preferred();

      xml = XmlDocument::LoadFromFileAsync(
               StorageFolder::GetFolderFromPathAsync(full_path.native())
                  .get()
                  .GetFileAsync(xml_manifest_file)
                  .get())
               .get();
   }
   catch (winrt::hresult_error&) {
      return {};
   }

   std::vector<std::filesystem::path> manifest;

   for (auto node : xml.SelectNodes(L"InstallInfo/installedFiles/item/key/string"sv)) {
      auto file_name = node.InnerText();

      manifest.emplace_back(std::wstring_view{file_name});
   }

   manifest.emplace_back(base_path / xml_manifest_directory / xml_manifest_file);

   return manifest;
}
