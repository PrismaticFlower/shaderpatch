
#include "ucfb_editor.hpp"

namespace sp::ucfb {

Editor_data_writer::Editor_data_writer(Editor_data_chunk& data_chunk) noexcept
   : _data{data_chunk}
{
}

void Editor_data_writer::write(const std::span<const std::byte> span, Alignment alignment)
{
   _data.insert(_data.cend(), span.begin(), span.end());

   if (alignment == Alignment::aligned) align();
}

void Editor_data_writer::write(const std::string_view string, Alignment alignment)
{
   const auto bytes = std::as_bytes(std::span{string});

   _data.insert(_data.cend(), bytes.begin(), bytes.end());
   _data.push_back(std::byte{'\0'});

   if (alignment == Alignment::aligned) align();
}

void Editor_data_writer::write(const std::string& string, Alignment alignment)
{
   write(std::string_view{string}, alignment);
}

void Editor_data_writer::align() noexcept
{
   const auto remainder = _data.size() % 4u;

   if (remainder != 0) {
      const std::array<std::byte, 4> nulls{};

      write(std::span{nulls.data(), (4 - remainder)}, Alignment::unaligned);
   }
}

}
