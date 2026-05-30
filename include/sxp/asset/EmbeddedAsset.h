//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_EMBEDDEDASSET_H
#define LIBSXP_EMBEDDEDASSET_H

#include <cstdint>
#include <string>
#include <array>

#include "AssetId.h"
#include "AssetKind.h"

namespace sxp {
  struct EmbeddedAsset {
    AssetId id;

    AssetKind kind = AssetKind::Unknown;

    std::string displayName;
    std::string originalPathHint;
    std::string mimeType;

    std::uint64_t blobChunkIndex = 0;

    std::uint64_t storedSize = 0;
    std::uint64_t uncompressedSize = 0;

    std::uint32_t flags = 0;
    std::uint64_t checksum = 0;
  };
}

#endif //LIBSXP_EMBEDDEDASSET_H
