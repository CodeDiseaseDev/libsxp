//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_ASSETKIND_H
#define LIBSXP_ASSETKIND_H
#include <cstdint>

namespace sxp {
  enum class AssetKind : std::uint32_t {
    Unknown = 0,
    AudioSample = 1,
    Image = 2,
    Preset = 3,
    Text = 4,
    Binary = 5
  };

}

#endif //LIBSXP_ASSETKIND_H
