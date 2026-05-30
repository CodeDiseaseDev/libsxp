//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_ASSETWRITER_H
#define LIBSXP_ASSETWRITER_H

#include "sxp/asset/AssetId.h"
#include "sxp/asset/AssetKind.h"
#include "sxp/asset/AssetManifest.h"
#include "sxp/core/Result.h"
#include "sxp/file/FileWriter.h"

#include <cstdint>
#include <string>

namespace sxp {
  class AssetWriter {
  public:
    Result<AssetId> EmbedFile(
      FileWriter& writer,
      const std::string& path,
      AssetKind kind,
      const std::string& mimeType
    );

    Error WriteManifest(FileWriter& writer);

    const AssetManifest& GetManifest() const;

  private:
    AssetManifest manifest_;
    std::uint64_t nextAssetId_ = 1;
  };
}

#endif // LIBSXP_ASSETWRITER_H