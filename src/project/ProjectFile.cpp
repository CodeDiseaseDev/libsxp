#include "sxp/project/ProjectFile.h"

#include <cstring>

#include "sxp/chunk/ChunkId.h"
#include "sxp/file/FileReader.h"
#include "sxp/file/FileWriter.h"
#include "sxp/project/ProjectDocumentCodec.h"
#include "sxp/stream/BinaryReader.h"
#include "sxp/stream/BinaryWriter.h"

#include <sstream>
#include <span>
#include <string>
#include <vector>

#include "sxp/sxp_format/SxpFormat.h"

namespace {
  std::vector<std::uint8_t> ToBytes(
    const std::string& value
  ) {
    return std::vector<std::uint8_t>(
      value.begin(),
      value.end()
    );
  }
}

namespace sxp {
  sxp::Error sxp::ProjectFile::Save(
    const std::string& path,
    const ProjectDocument& project
  ) {



    std::ostringstream projectStream(
      std::ios::out | std::ios::binary
    );

    BinaryWriter projectWriter(projectStream);

    auto err = ProjectDocumentCodec::Write(
      projectWriter,
      project
    );

    if (!err.Ok()) {
      return err;
    }

    const std::string projectData = projectStream.str();

    FileWriter fileWriter;

    err = fileWriter.Open(path);

    if (!err.Ok()) {
      return err;
    }

    err = fileWriter.WriteChunk(
      Chunk_PROJ,
      1,
      std::span<const std::uint8_t>(
        reinterpret_cast<const std::uint8_t*>(projectData.data()),
        projectData.size()
      )
    );

    if (!err.Ok()) {
      return err;
    }

    return fileWriter.Finalize();
  }

  sxp::Result<sxp::ProjectDocument> sxp::ProjectFile::Load(
    const std::string& path,
    ReadLimits limits
  ) {
    FileReader fileReader;

    auto err = fileReader.Open(path, limits);

    if (!err.Ok()) {
      return Result<ProjectDocument> {
        {},
        err
      };
    }

    const ChunkEntry* projectChunk =
      fileReader
        .GetTableOfContents()
        .FindFirst(Chunk_PROJ);

    if (projectChunk == nullptr) {
      return Result<ProjectDocument> {
        {},
        Fail(
          ErrorCode::MissingRequiredChunk,
          "SXP file is missing the PROJ chunk."
        )
      };
    }

    auto payloadResult =
      fileReader.ReadChunkPayload(*projectChunk);

    if (!payloadResult.Ok()) {
      return Result<ProjectDocument> {
        {},
        payloadResult.error
      };
    }

    const auto& payload = payloadResult.value;

    std::string projectData(
      reinterpret_cast<const char*>(payload.data()),
      payload.size()
    );

    std::istringstream projectStream(
      projectData,
      std::ios::in | std::ios::binary
    );

    BinaryReader projectReader(projectStream, limits);

    return ProjectDocumentCodec::Read(projectReader);
  }
}
