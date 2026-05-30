//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_PROJECTHEADER_H
#define LIBSXP_PROJECTHEADER_H
#include <cstdint>
#include <string>

namespace sxp {
  struct ProjectHeader {
    // std::uint32_t projectFormatVersion = 1;

    std::uint64_t projectIdHigh;
    std::uint64_t projectIdLow;

    std::string projectName;
    double bpm;

    std::uint32_t timeSigNumerator;
    std::uint32_t timeSigDenominator;

    double swingAmount;
    double swingSubdivisionBeats;
  };
}

#endif //LIBSXP_PROJECTHEADER_H
