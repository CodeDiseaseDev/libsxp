//
// Created by code on 6/1/26.
//

#ifndef SYNTHEM_PROJECTASSET_H
#define SYNTHEM_PROJECTASSET_H
#include "sxp/asset/AssetId.h"
#include "sxp/asset/AssetKind.h"
#include <string>
#include <vector>

namespace sxp {
  struct ProjectAsset {
    AssetId id {};
    AssetKind kind = AssetKind::Unknown;

    std::string displayName;
    std::string originalPathHint;
    std::string mimeType;

    std::vector<std::uint8_t> data;
  };
}

#endif //SYNTHEM_PROJECTASSET_H
