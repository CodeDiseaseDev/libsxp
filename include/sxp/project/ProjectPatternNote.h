//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_PROJECTPATTERNNOTE_H
#define LIBSXP_PROJECTPATTERNNOTE_H
#include <cstdint>

namespace sxp {
  struct ProjectPatternNote {
    std::uint64_t id = 0;

    std::int32_t midiNote = 60;
    float velocity = 1.0f;

    double startBeat = 0.0;
    double lengthBeats = 1.0;
  };
}

#endif //LIBSXP_PROJECTPATTERNNOTE_H
