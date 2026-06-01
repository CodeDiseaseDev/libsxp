#ifndef LIBSXP_PROJECTDOCUMENTCODEC_H
#define LIBSXP_PROJECTDOCUMENTCODEC_H

#include "sxp/core/Error.h"
#include "sxp/core/Result.h"
#include "sxp/project/ProjectDocument.h"
#include "sxp/stream/BinaryReader.h"
#include "sxp/stream/BinaryWriter.h"

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

    static Error WriteAssetsSection(
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

    static Error ReadAssetsSection(
      BinaryReader& reader,
      ProjectDocument& project
    );
  };
}

#endif // LIBSXP_PROJECTDOCUMENTCODEC_H