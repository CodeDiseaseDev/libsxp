//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_ASSETID_H
#define LIBSXP_ASSETID_H
#include <cstdint>

namespace sxp {
  struct AssetId {
    std::uint64_t high = 0;
    std::uint64_t low = 0;
  };
}

#endif //LIBSXP_ASSETID_H
