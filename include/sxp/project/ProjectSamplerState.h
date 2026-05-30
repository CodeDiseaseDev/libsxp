//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_PROJECTSAMPLERSTATE_H
#define LIBSXP_PROJECTSAMPLERSTATE_H
#include <cstdint>

#include "sxp/asset/AssetId.h"

namespace sxp {
  struct ProjectSamplerState {
    AssetId sampleAssetId;

    std::int32_t rootNote = 60;
    bool pitchFollowsNote = false;
    bool oneShot = true;

    float gain = 1.0f;
  };
}

#endif //LIBSXP_PROJECTSAMPLERSTATE_H
