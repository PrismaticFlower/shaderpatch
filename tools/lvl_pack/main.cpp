
#include "ucfb_reader.hpp"
#include "ucfb_writer.hpp"

#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include <boost/filesystem.hpp>
#include <gsl/gsl>

namespace fs = boost::filesystem;

using namespace sp;
using namespace std::literals;

namespace {
auto read_binary_in(const fs::path& path) -> std::vector<std::byte>
{
   if (!fs::is_regular_file(path)) {
      throw std::runtime_error{"File listed in definition does not exist."s};
   }

   std::ifstream file{path.string(), std::ios::binary | std::ios::ate};

   const auto file_size = file.tellg();

   std::vector<std::byte> vector;
   vector.resize(static_cast<std::size_t>(file_size));

   file.seekg(0);
   file.read(reinterpret_cast<char*>(vector.data()), vector.size());

   return vector;
}
}

int main(int arg_count, char* args[])
{
   if (arg_count != 4) {
      std::cout << "usage: <definition path> <munged files path> <out path>\n";

      return 1;
   }

   const fs::path def_path = args[1];
   const fs::path munged_files_path = args[2];
   const fs::path out_path = args[3];

   try {
      if (!fs::is_regular_file(def_path)) {
         throw std::runtime_error{"Definition file is not a file."};
      }

      if (!fs::is_directory(munged_files_path)) {
         throw std::runtime_error{
            "Supplied path to munged files is not a directory."};
      }

      fs::create_directory(out_path.parent_path());

      auto output_file = ucfb::open_file_for_output(out_path.string());

      ucfb::Writer writer{output_file};

      for (std::ifstream definition{def_path.string()}; definition;) {
         std::string filename;
         std::getline(definition, filename);

         if (filename.length() == 0 || filename[0] == '#') continue;

         try {
            const auto file =
               read_binary_in(fs::path{munged_files_path}.append(filename));

            ucfb::Reader reader{gsl::make_span(file)};

            writer.write(reader.read_array<std::byte>(reader.size()));
         }
         catch (std::exception& e) {
            std::cout << "Error occured while processing file "sv
                      << std::quoted(filename) << '\n';

            throw e;
         }
      }
   }
   catch (std::exception& e) {
      std::cout << "Packing LVL Failed: " << e.what() << '\n';

      return 1;
   }
}
