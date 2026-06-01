//
// Created by code on 6/1/26.
//

#ifndef LIBSXP_SAMPLERPAYLOAD_H
#define LIBSXP_SAMPLERPAYLOAD_H
#include <string>
#include <cstdint>

#include "sxp/asset/AssetId.h"

namespace sxp {
  struct ProjectSamplerPayloadV1 {
    AssetId sampleAssetId {};
    std::string samplePath;

    std::int32_t rootNote = 60;
    bool pitchFollow = true;
    bool oneShot = true;

    std::uint64_t mixerChannelId = 1;
  };
}

#endif //LIBSXP_SAMPLERPAYLOAD_H
