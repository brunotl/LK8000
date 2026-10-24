// Copyright (c) 2024, Bruno de Lacheisserie
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//  * Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of  nor the names of its contributors may be used to
//    endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef _utils_uuid_h_
#define _utils_uuid_h_

#include "parse_hex.h"
#include <format>
#include <stdexcept>
#include <span>
#include <string>
#include <string_view>

#ifdef uuid_t
// this is required to solve conflict on win32 platform
#undef uuid_t
#endif

/*
    RFC 9562 - Universally Unique IDentifiers (UUIDs)

    The formal definition of the UUID string representation is provided by
    the following ABNF:

    UUID     = 4hexOctet "-"
               2hexOctet "-"
               2hexOctet "-"
               2hexOctet "-"
               6hexOctet
    hexOctet = HEXDIG HEXDIG
    DIGIT    = %x30-39
    HEXDIG   = DIGIT / "A" / "B" / "C" / "D" / "E" / "F
 */

class uuid_t {
 public:
  uuid_t() = default;
  constexpr uuid_t(uuid_t&&) = default;
  constexpr uuid_t(const uuid_t&) = default;

  constexpr uuid_t(const char (&string)[37]) noexcept
      : uuid_t(uuid_msb(string), uuid_lsb(string)) {}

  constexpr uuid_t(uint64_t msb, uint64_t lsb) noexcept
      : _msb(msb), _lsb(lsb) {}

  // Load from big-endian byte array
  constexpr uuid_t(const std::span<const uint8_t, 16>& bytes) noexcept
      : _msb(load_uuid_half(bytes.template subspan<0, 8>())),
        _lsb(load_uuid_half(bytes.template subspan<8, 8>())) {}

  constexpr uuid_t(std::string_view string)
      : uuid_t(uuid_msb(string), uuid_lsb(string)) {}

  uuid_t& operator=(uuid_t&&) noexcept = default;
  uuid_t& operator=(const uuid_t&) noexcept = default;

  constexpr auto operator<=>(const uuid_t& uuid) const noexcept = default;

  size_t hash() const noexcept {
      return std::hash<uint64_t>{}(_msb) ^ std::hash<uint64_t>{}(_lsb);
  }

  constexpr uint64_t msb() const noexcept {
    return _msb;
  }

  constexpr uint64_t lsb() const noexcept {
    return _lsb;
  }

  std::string to_string() const {
    return std::format("{:08x}-{:04x}-{:04x}-{:04x}-{:012x}",
                       static_cast<uint32_t>((_msb >> 32) & 0x0FFFFFFFF),
                       static_cast<uint16_t>((_msb >> 16) & 0x0FFFF),
                       static_cast<uint16_t>(_msb & 0x0FFFF),
                       static_cast<uint16_t>((_lsb >> 48) & 0x0FFFF),
                       static_cast<uint64_t>(_lsb & 0x0FFFFFFFFFFFFULL));
  }

 private:
  uint64_t _msb;
  uint64_t _lsb;

  constexpr static
  uint64_t uuid_msb(const char* str) {
    uint64_t msb = hex::to_uint32_t(str);  // 8 digit
    msb <<= 16;
    msb |= hex::to_uint16_t(str + 9);  // 4 digit
    msb <<= 16;
    msb |= hex::to_uint16_t(str + 14);  // 4 digit
    return msb;
  }

  constexpr static
  uint64_t uuid_lsb(const char* str) {
    uint64_t lsb = hex::to_uint16_t(str + 19);  // 4 digit
    lsb <<= 16;
    lsb |= hex::to_uint16_t(str + 24);  // 4 digit
    lsb <<= 32;
    lsb |= hex::to_uint32_t(str + 28);  // 8 digit
    return lsb;
  }

  constexpr static
  uint64_t uuid_msb(const std::string_view& str) {
    if (str.size() < 36) {
      throw std::invalid_argument("Invalid UUID string");
    }
    return uuid_msb(str.data());
  }

  constexpr static
  uint64_t uuid_lsb(const std::string_view& str) {
    if (str.size() < 36) {
      throw std::invalid_argument("Invalid UUID string");
    }
    return uuid_lsb(str.data());
  }

  constexpr static
  uint64_t load_uuid_half(const std::span<const uint8_t, 8>& bytes) {
    uint64_t half = 0;
    for (size_t i = 0; i < bytes.size(); ++i) {
      half <<= 8;
      half |= bytes[i];
    }
    return half;
  }
};

struct uuid_hash {
  size_t operator()(uuid_t uuid) const {
    return uuid.hash();
  }
};

#endif  // _utils_uuid_h_
