//
// Created by code on 6/1/26.
//

#ifndef LIBSXP_OSCILLATORPAYLOAD_H
#define LIBSXP_OSCILLATORPAYLOAD_H
#include <cstdint>

namespace sxp {
  struct ProjectOscillatorPayloadV1 {
    std::uint32_t waveShape = 0;
    std::int32_t octave = 0;
    std::int32_t semitone = 0;

    float attackSeconds = 0.005f;
    float decaySeconds = 0.25f;
    float sustainLevel = 1.0f;
    float releaseSeconds = 0.005f;

    std::uint64_t mixerChannelId = 1;
  };
}

#endif //LIBSXP_OSCILLATORPAYLOAD_H
