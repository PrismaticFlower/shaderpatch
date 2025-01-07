#pragma once

#include <string_view>
#include <vector>

#include <absl/container/flat_hash_map.h>

namespace sp::shadows {

/// @brief Stores a table to translate name hashes to strings for printing/UI.
struct Name_table {
   /// @brief Clear the table.
   void clear() noexcept;

   /// @brief Add a new name to the table.
   /// @param name_string The name to add.
   /// @return The hash of the name, returned for convenience.
   auto add(const std::string_view name_string) noexcept -> std::uint32_t;

   /// @brief Lookup a name in the table.
   /// @param name_hash The hash of the name.
   /// @return The name as a null terminated string or an empty, but valid null terminated string.
   auto lookup(const std::uint32_t name_hash) const noexcept -> const char*;

   /// @brief Get an iterator for visualizing the table.
   /// @return The begin iterator.
   auto begin() const noexcept
      -> absl::flat_hash_map<std::uint32_t, const char*>::const_iterator;

   /// @brief Get the iterator for visualizing the table.
   /// @return The end iterator.
   auto end() const noexcept
      -> absl::flat_hash_map<std::uint32_t, const char*>::const_iterator;

   /// @brief Returns the approximate count for how much memory has been allocated by the Name_table.
   /// @return The approximate count of allocated bytes.
   auto allocated_bytes() const noexcept -> std::size_t;

private:
   auto get_storage_vector(const std::size_t size) noexcept -> std::vector<char>&;

   absl::flat_hash_map<std::uint32_t, const char*> _map;
   std::vector<std::vector<char>> _storage;
};

}