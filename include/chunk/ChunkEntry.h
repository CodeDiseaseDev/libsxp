//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_CHUNKENTRY_H
#define LIBSXP_CHUNKENTRY_H
#include <cstdint>

#include "ChunkId.h"

namespace sxp {
  struct ChunkEntry {
    FourCC id = 0;

    std::uint32_t version = 1;
    std::uint32_t flags = 0;

    std::uint64_t offset = 0;
    std::uint64_t storedSize = 0;
    std::uint64_t uncompressedSize = 0;

    std::uint64_t checksum = 0;
  };
}

#endif //LIBSXP_CHUNKENTRY_H
