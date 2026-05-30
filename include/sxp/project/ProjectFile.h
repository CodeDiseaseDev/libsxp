//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_PROJECTFILE_H
#define LIBSXP_PROJECTFILE_H
#include "ProjectDocument.h"
#include "sxp/core/Error.h"
#include "sxp/core/Result.h"
#include "sxp/stream/ReadLimits.h"

namespace sxp {
  class ProjectFile {
  public:
    static Error Save(
      const std::string& path,
      const ProjectDocument& project
    );

    static Result<ProjectDocument> Load(
      const std::string& path,
      ReadLimits limits = {}
    );
  };
}

#endif //LIBSXP_PROJECTFILE_H
