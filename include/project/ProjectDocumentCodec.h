#ifndef LIBSXP_PROJECTDOCUMENTCODEC_H
#define LIBSXP_PROJECTDOCUMENTCODEC_H

#include "core/Error.h"
#include "core/Result.h"
#include "project/ProjectDocument.h"
#include "stream/BinaryReader.h"
#include "stream/BinaryWriter.h"

namespace sxp {
  class ProjectDocumentCodec {
  public:
    static Error Write(
      BinaryWriter& writer,
      const ProjectDocument& project
    );

    static Result<ProjectDocument> Read(
      BinaryReader& reader
    );

  private:
    static Error WriteTracksSection(
      BinaryWriter& writer,
      const ProjectDocument& project
    );

    static Error WritePatternsSection(
      BinaryWriter& writer,
      const ProjectDocument& project
    );

    static Error WriteArrangementSection(
      BinaryWriter& writer,
      const ProjectDocument& project
    );

    static Error WriteGeneratorsSection(
      BinaryWriter& writer,
      const ProjectDocument& project
    );

    static Error ReadTracksSection(
      BinaryReader& reader,
      ProjectDocument& project
    );

    static Error ReadPatternsSection(
      BinaryReader& reader,
      ProjectDocument& project
    );

    static Error ReadArrangementSection(
      BinaryReader& reader,
      ProjectDocument& project
    );

    static Error ReadGeneratorsSection(
      BinaryReader& reader,
      ProjectDocument& project
    );
  };
}

#endif // LIBSXP_PROJECTDOCUMENTCODEC_H