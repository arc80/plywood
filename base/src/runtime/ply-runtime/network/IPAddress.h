/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/string/String.h>

namespace ply {

struct IPAddress {
  enum Version {
    V4,
    V6,
  };

  u32 netOrdered[4]; // big endian

  PLY_INLINE Version version() const {
    return (this->netOrdered[0] == 0 && this->netOrdered[1] == 0 &&
            this->netOrdered[2] == PLY_CONVERT_BIG_ENDIAN(0xffffu))
               ? V4
               : V6;
  }

  PLY_INLINE bool isNull() const {
    return this->netOrdered[0] == 0 && this->netOrdered[1] == 0 &&
           this->netOrdered[2] == 0 && this->netOrdered[3] == 0;
  }

  static constexpr PLY_INLINE IPAddress localHost(Version ipVersion) {
    return (ipVersion == Version::V4)
               ? IPAddress{{0, 0, PLY_CONVERT_BIG_ENDIAN(0xffffu),
                            PLY_CONVERT_BIG_ENDIAN(0x7f000001u)}}
               : IPAddress{{0, 0, 0, PLY_CONVERT_BIG_ENDIAN(1u)}};
  }

  static constexpr PLY_INLINE IPAddress fromIPv4(u32 netOrdered) {
    return {{0, 0, PLY_CONVERT_BIG_ENDIAN(0xffffu), netOrdered}};
  }

  String toString() const;
  static IPAddress fromString();
  PLY_INLINE static IPAddress resolveHostName(StringView hostName,
                                              Version ipVersion);
};

enum class IPResult : u8 {
  Unknown = 0,
  OK,
  NoSocket,
  Unreachable,
  Refused,
  InUse,
};

} // namespace ply
