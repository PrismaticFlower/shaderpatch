#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace sp::shadows {

struct Texture_hash {
   std::uint32_t words[8] = {};

   bool operator==(const Texture_hash& other) const noexcept
   {
      return this->words[0] == other.words[0] && this->words[1] == other.words[1] &&
             this->words[2] == other.words[2] && this->words[3] == other.words[3] &&
             this->words[4] == other.words[4] && this->words[5] == other.words[5] &&
             this->words[6] == other.words[6] && this->words[7] == other.words[7];
   }

   template<typename H>
   friend H AbslHashValue(H h, const Texture_hash& hash)
   {
      return H::combine(std::move(h), hash.words[0], hash.words[1],
                        hash.words[2], hash.words[3], hash.words[4],
                        hash.words[5], hash.words[6], hash.words[7]);
   }
};

struct Texture_hasher {
   void process_bytes(std::span<const std::byte> bytes) noexcept;

   auto result() -> Texture_hash;

private:
   std::uint32_t h0 = 0x6a09e667;
   std::uint32_t h1 = 0xbb67ae85;
   std::uint32_t h2 = 0x3c6ef372;
   std::uint32_t h3 = 0xa54ff53a;
   std::uint32_t h4 = 0x510e527f;
   std::uint32_t h5 = 0x9b05688c;
   std::uint32_t h6 = 0x1f83d9ab;
   std::uint32_t h7 = 0x5be0cd19;

   const static std::size_t chunk_size = 64;

   std::size_t pending_bytes_count = 0;
   std::array<std::byte, chunk_size> pending_bytes = {};

   std::size_t total_message_bytes = 0;

   void process_pending() noexcept;
};

}