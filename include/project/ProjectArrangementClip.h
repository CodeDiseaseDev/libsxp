//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_PROJECTARRANGEMENTCLIP_H
#define LIBSXP_PROJECTARRANGEMENTCLIP_H
#include <cstdint>

namespace sxp {
  struct ProjectArrangementClip {
    std::uint64_t id = 0;

    std::uint64_t trackId = 0;
    std::uint64_t patternId = 0;

    double startBeat = 0.0;
    double lengthBeats = 4.0;
    double patternStartBeat = 0.0;

    bool muted = false;
  };
}

#endif //LIBSXP_PROJECTARRANGEMENTCLIP_H
