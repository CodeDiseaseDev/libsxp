//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_PROJECTHEADER_H
#define LIBSXP_PROJECTHEADER_H
#include <cstdint>
#include <string>

namespace sxp {
  struct ProjectHeader {
    std::uint64_t projectIdHigh = 0;
    std::uint64_t projectIdLow = 0;

    std::string projectName;
    double bpm = 0;

    std::uint32_t timeSigNumerator = 0;
    std::uint32_t timeSigDenominator = 0;

    double swingAmount = 0;
    double swingSubdivisionBeats = 0;
  };
}

#endif //LIBSXP_PROJECTHEADER_H
