//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_READLIMITS_H
#define LIBSXP_READLIMITS_H
#include <cstdint>

namespace sxp {
  struct ReadLimits {
    std::uint64_t maxStringBytes = 1024 * 1024;
    std::uint64_t maxChunkBytes = 1024ull * 1024ull * 1024ull;
    std::uint64_t maxAssetBytes = 1024ull * 1024ull * 1024ull;
    std::uint64_t maxChunkCount = 100000;

    std::uint64_t maxFileBytes = 1024ull * 1024ull * 1024ull * 8ull;
    std::uint64_t maxAssetCount = 10000;
  };
}

#endif //LIBSXP_READLIMITS_H
