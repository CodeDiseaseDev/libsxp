//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_PROJECTSECTIONHEADER_H
#define LIBSXP_PROJECTSECTIONHEADER_H
#include <cstdint>

#include "ProjectSectionId.h"

namespace sxp {
  struct ProjectSectionHeader {
    ProjectSectionId id = ProjectSectionId::Tracks;
    std::uint32_t version = 1;
    std::uint64_t size = 0;
  };
}

#endif //LIBSXP_PROJECTSECTIONHEADER_H
