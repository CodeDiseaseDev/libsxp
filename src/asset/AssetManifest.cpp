#include "asset/AssetManifest.h"

namespace sxp {
  const EmbeddedAsset* AssetManifest::FindById(
    const AssetId& id
  ) const {
    for (const auto& asset : assets) {
      if (asset.id.high == id.high &&
          asset.id.low == id.low) {
        return &asset;
          }
    }

    return nullptr;
  }
}