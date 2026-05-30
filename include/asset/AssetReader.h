//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_ASSETREADER_H
#define LIBSXP_ASSETREADER_H

#include "asset/AssetId.h"
#include "asset/AssetManifest.h"
#include "core/Result.h"
#include "file/FileReader.h"

#include <cstdint>
#include <string>
#include <vector>

namespace sxp {
  class AssetReader {
  public:
    AssetReader(
      FileReader& reader,
      AssetManifest manifest
    );

    Result<std::vector<std::uint8_t>> ReadAssetBytes(
      const AssetId& id
    );

    Error ExtractAssetToFile(
      const AssetId& id,
      const std::string& safeOutputPath
    );

  private:
    FileReader& reader_;
    AssetManifest manifest_;
  };
}

#endif // LIBSXP_ASSETREADER_H