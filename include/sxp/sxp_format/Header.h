//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_HEADER_H
#define LIBSXP_HEADER_H
#include <cstdint>

namespace sxp {
  struct Header  {
    char magic[4] = {'S', 'X', 'P', '\0'};

    std::uint16_t majorVersion = 1;
    std::uint16_t minorVersion = 0;

    std::uint32_t flags = 0;

    std::uint64_t tocOffset = 0;
    std::uint64_t chunkCount = 0;
  };
}

#endif //LIBSXP_HEADER_H
