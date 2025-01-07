#include "name_table.hpp"

#include "swbf_fnv_1a.hpp"

namespace sp::shadows {

void Name_table::clear() noexcept
{
   _map.clear();
   _storage.clear();
}

auto Name_table::add(const std::string_view name_string) noexcept -> std::uint32_t
{
   const std::uint32_t name_hash = fnv_1a_hash(name_string);

   if (name_string.empty() || _map.contains(name_hash)) return name_hash;

   std::vector<char>& storage = get_storage_vector(name_string.size() + 1);

   const std::size_t pre_add_size = storage.size();

   storage.insert(storage.end(), name_string.begin(), name_string.end());
   storage.push_back('\0');

   const char* cstring = storage.data() + pre_add_size;

   _map.emplace(name_hash, cstring);

   return name_hash;
}

auto Name_table::lookup(const std::uint32_t name_hash) const noexcept -> const char*
{
   auto it = _map.find(name_hash);

   return it != _map.end() ? it->second : "";
}

auto Name_table::get_storage_vector(const std::size_t size) noexcept
   -> std::vector<char>&
{
   // We use vectors to manage storage here becuase they're easy and simple. However to make sure pointers inside _map don't get
   // invalidated they must have a fixed capacity and it must be allocated once when they're created.

   const std::size_t storage_page_size = 16384;

   if (_storage.empty()) {
      _storage.emplace_back().reserve(std::max(storage_page_size, size));
   }
   else if (size > (_storage.back().capacity() - _storage.back().size())) {
      _storage.emplace_back().reserve(std::max(storage_page_size, size));
   }

   return _storage.back();
}

auto Name_table::begin() const noexcept
   -> absl::flat_hash_map<std::uint32_t, const char*>::const_iterator
{
   return _map.begin();
}

auto Name_table::end() const noexcept
   -> absl::flat_hash_map<std::uint32_t, const char*>::const_iterator
{
   return _map.end();
}

auto Name_table::allocated_bytes() const noexcept -> std::size_t
{
   std::size_t count = _map.capacity() * sizeof(decltype(_map)::value_type);

   for (const std::vector<char>& allocation : _storage) {
      count += allocation.capacity();
   }

   return count;
}

}