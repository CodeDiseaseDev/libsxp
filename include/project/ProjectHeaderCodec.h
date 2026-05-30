//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_PROJECTHEADERCODEC_H
#define LIBSXP_PROJECTHEADERCODEC_H

#include "core/Error.h"
#include "core/Result.h"
#include "project/ProjectHeader.h"
#include "stream/BinaryReader.h"
#include "stream/BinaryWriter.h"

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
