//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_PROJECTSECTIONID_H
#define LIBSXP_PROJECTSECTIONID_H
#include <cstdint>

namespace sxp {
  enum class ProjectSectionId : std::uint32_t {
    Tracks = 1,
    Patterns = 2,
    Arrangement = 3,
    Generators = 4,
    Mixer = 5,
    Assets = 6,
    ViewState = 7
  };
}

#endif //LIBSXP_PROJECTSECTIONID_H
