//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_PROJECTHEADERCODEC_H
#define LIBSXP_PROJECTHEADERCODEC_H

#include "sxp/core/Error.h"
#include "sxp/core/Result.h"
#include "sxp/project/ProjectHeader.h"
#include "sxp/stream/BinaryReader.h"
#include "sxp/stream/BinaryWriter.h"

namespace sxp {
  class ProjectHeaderCodec {
  public:
    static Error Write(
      BinaryWriter& writer,
      const ProjectHeader& header
    );

    static Result<ProjectHeader> Read(
      BinaryReader& reader
    );
  };
}

#endif //LIBSXP_PROJECTHEADERCODEC_H
