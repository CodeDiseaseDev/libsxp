//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_CHECKSUMKIND_H
#define LIBSXP_CHECKSUMKIND_H
#include <cstdint>

namespace sxp {
  enum class ChecksumKind : std::uint32_t {
    None = 0,
    Crc64 = 1,
    Sha256 = 2
  };
}

#endif //LIBSXP_CHECKSUMKIND_H
