//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_PROJECTGENERATORTYPE_H
#define LIBSXP_PROJECTGENERATORTYPE_H
#include <cstdint>

namespace sxp {
  enum class ProjectGeneratorType : std::uint32_t {
    Unknown = 0,
    Oscillator = 1,
    Sampler = 2
  };
}

#endif //LIBSXP_PROJECTGENERATORTYPE_H
