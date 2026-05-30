//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_PROJECTPATTERN_H
#define LIBSXP_PROJECTPATTERN_H
#include <string>
#include <vector>

#include "ProjectPatternNote.h"

namespace sxp {
  struct ProjectPattern {
    std::uint64_t id = 0;
    std::string name;

    double lengthBeats = 4.0;

    std::vector<ProjectPatternNote> notes;
  };
}

#endif //LIBSXP_PROJECTPATTERN_H
