//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_ASSETMANIFEST_H
#define LIBSXP_ASSETMANIFEST_H
#include <vector>

#include "AssetId.h"
#include "EmbeddedAsset.h"

namespace sxp {
  struct AssetManifest {
    std::vector<EmbeddedAsset> assets;

    const EmbeddedAsset* FindById(const AssetId& id) const;
  };
}

#endif //LIBSXP_ASSETMANIFEST_H
