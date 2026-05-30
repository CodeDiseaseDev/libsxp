//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_PROJECTDOCUMENT_H
#define LIBSXP_PROJECTDOCUMENT_H
#include <vector>

#include "ProjectArrangementClip.h"
#include "ProjectGenerator.h"
#include "ProjectHeader.h"
#include "ProjectPattern.h"
#include "ProjectTrack.h"

namespace sxp {
  struct ProjectDocument {
    ProjectHeader header;

    std::vector<ProjectTrack> tracks;
    std::vector<ProjectPattern> patterns;
    std::vector<ProjectArrangementClip> arrangementClips;
    std::vector<ProjectGenerator> generators;
  };
}

#endif //LIBSXP_PROJECTDOCUMENT_H
