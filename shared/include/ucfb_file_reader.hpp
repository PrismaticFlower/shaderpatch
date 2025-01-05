#pragma once

#include "magic_number.hpp"

#include <cstdio>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include <fmt/format.h>

namespace sp::ucfb {

struct File_reader_child {
   /// @brief Create a child reader.
   /// @param chunk_mn The magic number of the child.
   /// @param chunk_size The size of the child.
   /// @param file The file the child belongs to, with it's read head at the child.
   /// @param chunk_offset For use in error messages. The (optional) absolute offset of the chunk in the file.
   /// @param parent For use in error messages. An (optional) pointer to the parent chunk.
   File_reader_child(const Magic_number chunk_mn, const std::uint32_t chunk_size,
                     std::FILE* file, std::uint64_t file_chunk_offset,
                     const File_reader_child* const parent = nullptr)
      : _mn{chunk_mn},
        _size{chunk_size},
        _file{file},
        _file_chunk_offset{file_chunk_offset},
        _parent{parent}
   {
   }

   /// @brief Destroys the child and fast forwards the file's read pointer to the end of the chunk if needed.
   ~File_reader_child()
   {
      if (_head < _size)
         std::fseek(_file, static_cast<long>(_size - _head), SEEK_CUR);

      // Chunks are aligned to 4 byte boundaries so move the file read head forward if needed.
      const std::uint64_t remainder = _size % 4;

      if (remainder == 0) return;

      const std::uint64_t align_amount = 4 - remainder;

      std::fseek(_file, static_cast<long>(align_amount), SEEK_CUR);
   }

   File_reader_child(const File_reader_child&) = delete;
   File_reader_child(File_reader_child&&) = delete;

   /// @brief Reads a trivial value from the chunk.
   ///
   /// @tparam Type The type of the value to read. The type must be trivially copyable.
   ///
   /// @return The read value.
   ///
   /// @exception std::runtime_error Thrown when reading the value went past the end
   /// of the chunk.
   template<typename Type>
   auto read() -> Type
   {
      static_assert(std::is_trivially_copyable_v<Type>,
                    "Type must be trivially copyable.");
      static_assert(!std::is_reference_v<Type>, "Type can not be a reference.");
      static_assert(!std::is_pointer_v<Type>, "Type can not be a pointer.");

      Type value = {};

      read_bytes_strict(std::as_writable_bytes(std::span{&value, 1}));

      return value;
   }

   /// @brief Reads a variable-length array of trivial values from the chunk.
   ///
   /// @tparam Type The type of the values to read. The type must be trivially copyable.
   ///
   /// @param out_bytes A span to read the array into. out_array.size() elements will be read.
   ///
   /// @exception std::runtime_error Thrown when reading the array went past the end
   /// of the chunk.
   template<typename Type>
   void read_array(std::span<Type> out_array)
   {
      static_assert(std::is_trivially_copyable_v<Type>,
                    "Type must be trivially copyable.");
      static_assert(!std::is_pointer_v<Type>, "Type can not be a pointer.");

      read_bytes_strict(std::as_writable_bytes(out_array));
   }

   /// @brief Attempt to read a span of bytes out of the chunk.
   ///
   /// @param out_bytes A span to read the bytes into. out_bytes.size() bytes will be read.
   ///
   /// @return True if out_bytes.size() were read, false if less than out_bytes.size() were read.
   bool read_bytes(std::span<std::byte> out_bytes) noexcept
   {
      if (_head + out_bytes.size() > _size) {
         const std::uint64_t overflow = _head + out_bytes.size() - _size;
         const std::uint64_t safe_read_size = out_bytes.size() - overflow;

         std::fill_n(out_bytes.data(), out_bytes.size(), std::byte{});

         if (safe_read_size == 0) return false;

         std::fread(out_bytes.data(), sizeof(std::byte),
                    static_cast<std::size_t>(safe_read_size), _file);

         _head = _size;

         return false;
      }
      else {
         _head += out_bytes.size();

         const bool result = std::fread(out_bytes.data(), sizeof(std::byte),
                                        out_bytes.size(), _file) == out_bytes.size();

         return result;
      }
   }

   /// @brief Attempt to read a span of bytes out of the chunk.
   ///
   /// @param out_bytes A span to read the bytes into. out_bytes.size() bytes will be read.
   ///
   /// @exception std::runtime_error Thrown when reading the bytes went past the end
   /// of the chunk. out_bytes may contain some bytes but not all.
   void read_bytes_strict(std::span<std::byte> out_bytes)
   {
      if (!read_bytes(out_bytes)) throw_overflow();
   }

   /// @brief Reads a null-terminated string from a chunk.
   ///
   /// @return A string.
   ///
   /// @exception std::runtime_error Thrown when reading the string would go
   /// past the end of the chunk.
   auto read_string() -> std::string
   {
      std::string string;

      for (char c = read<char>(); c != '\0'; c = read<char>()) {
         string.push_back(c);
      }

      return string;
   }

   /// @brief Reads a child chunk.
   ///
   /// @return The File_reader_child for the child chunk.
   ///
   /// @exception std::runtime_error Thrown when reading the child would go past
   /// the end of the current chunk.
   auto read_child() -> File_reader_child
   {
      const std::uint64_t child_file_offset = _file_chunk_offset + _head;

      const Magic_number child_magic_number = read<Magic_number>();
      const std::uint32_t child_size = read<std::uint32_t>();

      if (_head + child_size > _size) throw_overflow();

      _head += child_size;

      // Chunks are aligned to 4 byte boundaries so move our read head forward if needed.
      // The child takes care of moving the file's read head forward in it's destructor.
      if (const std::uint64_t remainder = child_size % 4; remainder != 0) {
         const std::uint64_t align_amount = 4 - remainder;

         if (_head + align_amount <= _size) _head += align_amount;
      }

      return {child_magic_number, child_size, _file, child_file_offset, this};
   }

   /// @brief Shifts the read head forward an amount of bytes.
   ///
   /// @param amount The amount to shift the head forward by.
   ///
   /// @exception std::runtime_error Thrown when the consume operation would go
   /// past the end of the chunk
   void consume(const std::size_t amount)
   {
      if (_head + amount > _size) {
         const std::uint64_t overflow = _head + amount - _size;
         const std::uint64_t safe_seek_size = amount - overflow;

         if (safe_seek_size == 0) return;

         std::fseek(_file, static_cast<long>(safe_seek_size), SEEK_CUR);

         _head = _size;
      }
      else {
         _head += amount;

         std::fseek(_file, amount, SEEK_CUR);
      }
   }

   /// @brief Tests if the end of the chunk has been reached or not.
   ///
   /// @return True if the end of the chunk has not been reached, false if it has.
   explicit operator bool() const noexcept
   {
      return (_head < _size);
   }

   /// @brief Gets the magic number of the chunk.
   ///
   /// @return The magic number of the chunk.
   auto magic_number() const noexcept -> Magic_number
   {
      return _mn;
   }

   /// @brief Gets size (in bytes) of the chunk.
   ///
   /// @return The size the chunk.
   auto size() const noexcept -> std::size_t
   {
      return static_cast<std::size_t>(_size);
   }

protected:
   [[noreturn]] void throw_overflow()
   {
      std::vector<const File_reader_child*> parent_stack;
      parent_stack.reserve(16);

      const File_reader_child* parent = this;

      while (parent != nullptr) {
         parent_stack.push_back(parent);
         parent = parent->_parent;
      }

      std::string message = "Went past chunk bounds while reading. Trace:\n";
      message.reserve(256);

      const std::size_t indention_size = 4;
      std::size_t indention = 0;

      for (std::ptrdiff_t i = std::ssize(parent_stack) - 1; i >= 0; --i) {
         const File_reader_child* chunk = parent_stack[i];

         message.append(indention_size * 4, ' ');
         message +=
            fmt::format("{} {} (read offset: {})\n",
                        std::string_view{reinterpret_cast<const char*>(&chunk->_mn),
                                         sizeof(chunk->_mn)},
                        chunk->_file_chunk_offset,
                        chunk->_file_chunk_offset + chunk->_head);

         indention += 1;
      }

      throw std::runtime_error{message};
   }

   const Magic_number _mn;
   const std::uint64_t _size;
   std::FILE* const _file = nullptr;

   std::uint64_t _head = 0;

   const std::uint64_t _file_chunk_offset = 0;
   const File_reader_child* const _parent = nullptr;
};

struct File_reader : private File_reader_child {
   File_reader(const std::string& file_name)
      : File_reader_child{open_root_reader(file_name)}
   {
   }

   ~File_reader()
   {
      if (_file) std::fclose(_file);
   }

   File_reader(const File_reader&) = delete;
   File_reader(File_reader&&) = delete;

   using File_reader_child::read_child;
   using File_reader_child::operator bool;

private:
   static auto open_root_reader(const std::string& file_name) -> File_reader_child
   {
      std::FILE* file = nullptr;

      if (fopen_s(&file, file_name.c_str(), "rbS") != errno_t{}) {
         throw std::runtime_error{"Couldn't open file!"};
      }

      Magic_number magic_number = {};
      std::uint32_t size = 0;

      if (std::fread(&magic_number, sizeof(magic_number), 1, file) != 1) {
         throw std::runtime_error{"File too small!"};
      }

      if (std::fread(&size, sizeof(size), 1, file) != 1) {
         throw std::runtime_error{"File too small!"};
      }

      return {magic_number, size, file, 0};
   }
};

}