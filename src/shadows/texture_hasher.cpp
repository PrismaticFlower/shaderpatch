#include "texture_hasher.hpp"

#include <cassert>

namespace sp::shadows {

namespace {

using uint32 = std::uint32_t;

constexpr std::array<uint32, 64> k =
   {0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
    0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
    0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
    0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
    0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
    0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

/// @brief Hacked together SHA256 implementation from reading Wikipedia. Don't
/// use this for anything serious, it almost certainly isn't good enough.
void process_chunk_sha256(std::span<uint32, 64> words, uint32& h0, uint32& h1,
                          uint32& h2, uint32& h3, uint32& h4, uint32& h5,
                          uint32& h6, uint32& h7) noexcept
{
   for (uint32 i = 0; i < 16; ++i) {
      words[i] = _byteswap_ulong(words[i]);
   }

   for (uint32 i = 16; i < 64; ++i) {
      uint32 s0 = _rotr(words[i - 15], 7) xor _rotr(words[i - 15], 18) xor
                  (words[i - 15] >> 3);
      uint32 s1 = _rotr(words[i - 2], 17) xor _rotr(words[i - 2], 19) xor
                  (words[i - 2] >> 10);

      words[i] = words[i - 16] + s0 + words[i - 7] + s1;
   }

   uint32 a = h0;
   uint32 b = h1;
   uint32 c = h2;
   uint32 d = h3;
   uint32 e = h4;
   uint32 f = h5;
   uint32 g = h6;
   uint32 h = h7;

   for (uint32 i = 0; i < 64; ++i) {
      uint32 S1 = _rotr(e, 6) xor _rotr(e, 11) xor _rotr(e, 25);
      uint32 ch = (e bitand f) xor ((compl e) bitand g);
      uint32 temp1 = h + S1 + ch + k[i] + words[i];
      uint32 S0 = _rotr(a, 2) xor _rotr(a, 13) xor _rotr(a, 22);
      uint32 maj = (a bitand b) xor (a bitand c) xor (b bitand c);
      uint32 temp2 = S0 + maj;

      h = g;
      g = f;
      f = e;
      e = d + temp1;
      d = c;
      c = b;
      b = a;
      a = temp1 + temp2;
   }

   h0 += a;
   h1 += b;
   h2 += c;
   h3 += d;
   h4 += e;
   h5 += f;
   h6 += g;
   h7 += h;
}

}

void Texture_hasher::process_bytes(std::span<const std::byte> bytes) noexcept
{
   std::size_t processed_bytes = 0;

   while (processed_bytes != bytes.size()) {
      if (pending_bytes_count < chunk_size) {
         const std::size_t round_bytes = std::min(chunk_size - pending_bytes_count,
                                                  bytes.size() - processed_bytes);

         std::memcpy(&pending_bytes[pending_bytes_count],
                     &bytes[processed_bytes], round_bytes);

         pending_bytes_count += round_bytes;
         processed_bytes += round_bytes;
      }
      else {
         assert(pending_bytes_count == 0);

         const std::size_t round_bytes =
            std::min(bytes.size() - processed_bytes, chunk_size);

         std::memcpy(&pending_bytes[pending_bytes_count],
                     &bytes[processed_bytes], round_bytes);

         pending_bytes_count += round_bytes;
         processed_bytes += round_bytes;
      }

      if (pending_bytes_count == chunk_size) process_pending();
   }

   total_message_bytes += bytes.size();
}

auto Texture_hasher::result() -> Texture_hash
{
   const std::uint64_t ml = _byteswap_uint64(total_message_bytes * 8);

   bool need_bit_marker_tail = true;
   bool need_size_tail = true;

   if (pending_bytes_count > 0) {
      assert(pending_bytes_count < chunk_size);

      std::array<uint32, 64> words = {};

      std::memcpy(&words[0], pending_bytes.data(), pending_bytes_count);

      const std::uint8_t marker = 0x80;

      std::memcpy(reinterpret_cast<char*>(&words[0]) + pending_bytes_count,
                  &marker, sizeof(marker));

      need_bit_marker_tail = false;

      if (64 - pending_bytes_count - 1 >= 8) {
         std::memcpy(&words[14], &ml, sizeof(ml));

         need_size_tail = false;
      }

      process_chunk_sha256(words, h0, h1, h2, h3, h4, h5, h6, h7);
   }

   if (need_size_tail) {
      std::array<uint32, 64> words = {};

      if (need_bit_marker_tail) {
         const std::uint8_t marker = 0x80;

         std::memcpy(&words[0], &marker, sizeof(marker));
      }

      std::memcpy(&words[14], &ml, sizeof(ml));

      process_chunk_sha256(words, h0, h1, h2, h3, h4, h5, h6, h7);
   }

   return {h0, h1, h2, h3, h4, h5, h6, h7};
}

void Texture_hasher::process_pending() noexcept
{
   std::array<uint32, 64> words;

   std::memcpy(&words[0], pending_bytes.data(), chunk_size);

   process_chunk_sha256(words, h0, h1, h2, h3, h4, h5, h6, h7);

   pending_bytes_count = 0;
}

}