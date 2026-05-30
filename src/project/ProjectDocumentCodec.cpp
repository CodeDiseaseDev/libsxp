#include "project/ProjectDocumentCodec.h"

#include "project/ProjectArrangementClip.h"
#include "project/ProjectHeaderCodec.h"
#include "project/ProjectPattern.h"
#include "project/ProjectPatternNote.h"
#include "project/ProjectSectionHeader.h"
#include "project/ProjectSectionId.h"
#include "project/ProjectTrack.h"

#include <cstdint>
#include <functional>
#include <limits>
#include <sstream>
#include <span>
#include <string>
#include <vector>
#include "project/ProjectGenerator.h"
#include "project/ProjectGeneratorType.h"

namespace {
constexpr std::uint64_t MaxProjectSectionItems = 1'000'000;

template<typename T>
sxp::Result<T> MakeErrorResult(sxp::Error error) {
  sxp::Result<T> result;
  result.error = std::move(error);
  return result;
}

std::vector<std::uint8_t> ToBytes(const std::string& value) {
  return std::vector<std::uint8_t>(
    value.begin(),
    value.end()
  );
}

sxp::Error WriteSectionHeader(
  sxp::BinaryWriter& writer,
  const sxp::ProjectSectionHeader& header
) {
  auto err = writer.WriteU32(
    static_cast<std::uint32_t>(header.id)
  );
  if (!err.Ok()) return err;

  err = writer.WriteU32(header.version);
  if (!err.Ok()) return err;

  err = writer.WriteU64(header.size);
  if (!err.Ok()) return err;

  return sxp::Success();
}

sxp::Result<sxp::ProjectSectionHeader> ReadSectionHeader(
  sxp::BinaryReader& reader
) {
  sxp::ProjectSectionHeader header;

  std::uint32_t id = 0;

  auto err = reader.ReadU32(id);
  if (!err.Ok()) {
    return MakeErrorResult<sxp::ProjectSectionHeader>(err);
  }

  header.id = static_cast<sxp::ProjectSectionId>(id);

  err = reader.ReadU32(header.version);
  if (!err.Ok()) {
    return MakeErrorResult<sxp::ProjectSectionHeader>(err);
  }

  err = reader.ReadU64(header.size);
  if (!err.Ok()) {
    return MakeErrorResult<sxp::ProjectSectionHeader>(err);
  }

  return sxp::Result<sxp::ProjectSectionHeader> {
    header,
    sxp::Success()
  };
}

template<typename WritePayloadFunc>
sxp::Result<std::vector<std::uint8_t>> BuildSectionPayload(
  WritePayloadFunc&& writePayload
) {
  std::ostringstream stream(
    std::ios::out | std::ios::binary
  );

  sxp::BinaryWriter writer(stream);

  auto err = writePayload(writer);

  if (!err.Ok()) {
    return MakeErrorResult<std::vector<std::uint8_t>>(err);
  }

  return sxp::Result<std::vector<std::uint8_t>> {
    ToBytes(stream.str()),
    sxp::Success()
  };
}

sxp::Error WriteSection(
  sxp::BinaryWriter& writer,
  sxp::ProjectSectionId id,
  std::uint32_t version,
  std::span<const std::uint8_t> payload
) {
  sxp::ProjectSectionHeader header;
  header.id = id;
  header.version = version;
  header.size = static_cast<std::uint64_t>(payload.size());

  auto err = WriteSectionHeader(writer, header);

  if (!err.Ok()) {
    return err;
  }

  return writer.WriteBytes(payload);
}

sxp::Result<std::vector<std::uint8_t>> ReadSectionPayload(
  sxp::BinaryReader& reader,
  const sxp::ProjectSectionHeader& header
) {
  if (header.size >
      static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
    return MakeErrorResult<std::vector<std::uint8_t>>(
      sxp::Fail(
        sxp::ErrorCode::SizeLimitExceeded,
        "Project section payload is too large for this platform."
      )
    );
  }

  std::vector<std::uint8_t> payload;
  payload.resize(static_cast<std::size_t>(header.size));

  if (payload.empty()) {
    return sxp::Result<std::vector<std::uint8_t>> {
      payload,
      sxp::Success()
    };
  }

  auto err = reader.ReadBytes(
    std::span<std::uint8_t>(
      payload.data(),
      payload.size()
    )
  );

  if (!err.Ok()) {
    return MakeErrorResult<std::vector<std::uint8_t>>(err);
  }

  return sxp::Result<std::vector<std::uint8_t>> {
    payload,
    sxp::Success()
  };
}

sxp::BinaryReader MakeReaderForPayload(
  std::istringstream& stream
) {
  return sxp::BinaryReader(stream);
}

sxp::Error CheckCount(std::uint64_t count, const char* name) {
  if (count > MaxProjectSectionItems) {
    return sxp::Fail(
      sxp::ErrorCode::SizeLimitExceeded,
      std::string(name) + " count exceeded project section limit."
    );
  }

  return sxp::Success();
}
}

namespace sxp {

Error ProjectDocumentCodec::WriteGeneratorsSection(
  BinaryWriter& writer,
  const ProjectDocument& project
) {
  auto payloadResult = BuildSectionPayload(
    [&project](BinaryWriter& sectionWriter) {
      auto err = sectionWriter.WriteU64(
        static_cast<std::uint64_t>(project.generators.size())
      );
      if (!err.Ok()) return err;

      for (const auto& generator : project.generators) {
        err = sectionWriter.WriteU64(generator.nodeId);
        if (!err.Ok()) return err;

        err = sectionWriter.WriteString(generator.name);
        if (!err.Ok()) return err;

        err = sectionWriter.WriteU32(
          static_cast<std::uint32_t>(generator.type)
        );
        if (!err.Ok()) return err;

        err = sectionWriter.WriteU32(generator.version);
        if (!err.Ok()) return err;

        err = sectionWriter.WriteU64(
          static_cast<std::uint64_t>(generator.payload.size())
        );
        if (!err.Ok()) return err;

        if (!generator.payload.empty()) {
          err = sectionWriter.WriteBytes(
            std::span<const std::uint8_t>(
              generator.payload.data(),
              generator.payload.size()
            )
          );
          if (!err.Ok()) return err;
        }
      }

      return Success();
    }
  );

  if (!payloadResult.Ok()) {
    return payloadResult.error;
  }

  return WriteSection(
    writer,
    ProjectSectionId::Generators,
    1,
    std::span<const std::uint8_t>(
      payloadResult.value.data(),
      payloadResult.value.size()
    )
  );
}

Error ProjectDocumentCodec::ReadGeneratorsSection(
  BinaryReader& reader,
  ProjectDocument& project
) {
  std::uint64_t count = 0;

  auto err = reader.ReadU64(count);
  if (!err.Ok()) return err;

  err = CheckCount(count, "Generator");
  if (!err.Ok()) return err;

  project.generators.clear();
  project.generators.reserve(static_cast<std::size_t>(count));

  for (std::uint64_t i = 0; i < count; ++i) {
    ProjectGenerator generator;

    err = reader.ReadU64(generator.nodeId);
    if (!err.Ok()) return err;

    err = reader.ReadString(generator.name);
    if (!err.Ok()) return err;

    std::uint32_t type = 0;

    err = reader.ReadU32(type);
    if (!err.Ok()) return err;

    generator.type = static_cast<ProjectGeneratorType>(type);

    err = reader.ReadU32(generator.version);
    if (!err.Ok()) return err;

    std::uint64_t payloadSize = 0;

    err = reader.ReadU64(payloadSize);
    if (!err.Ok()) return err;

    err = CheckCount(payloadSize, "Generator payload byte");
    if (!err.Ok()) return err;

    generator.payload.clear();
    generator.payload.resize(static_cast<std::size_t>(payloadSize));

    if (!generator.payload.empty()) {
      err = reader.ReadBytes(
        std::span<std::uint8_t>(
          generator.payload.data(),
          generator.payload.size()
        )
      );
      if (!err.Ok()) return err;
    }

    project.generators.push_back(std::move(generator));
  }

  return Success();
}

Error ProjectDocumentCodec::Write(
  BinaryWriter& writer,
  const ProjectDocument& project
) {
  auto err = ProjectHeaderCodec::Write(
    writer,
    project.header
  );

  if (!err.Ok()) {
    return err;
  }

  constexpr std::uint32_t sectionCount = 4;

  err = writer.WriteU32(sectionCount);
  if (!err.Ok()) return err;

  err = WriteTracksSection(writer, project);
  if (!err.Ok()) return err;

  err = WritePatternsSection(writer, project);
  if (!err.Ok()) return err;

  err = WriteArrangementSection(writer, project);
  if (!err.Ok()) return err;

  err = WriteGeneratorsSection(writer, project);
  if (!err.Ok()) return err;

  return Success();
}

Result<ProjectDocument> ProjectDocumentCodec::Read(
  BinaryReader& reader
) {
  ProjectDocument project;

  auto headerResult = ProjectHeaderCodec::Read(reader);

  if (!headerResult.Ok()) {
    return MakeErrorResult<ProjectDocument>(headerResult.error);
  }

  project.header = std::move(headerResult.value);

  std::uint32_t sectionCount = 0;

  auto err = reader.ReadU32(sectionCount);

  if (!err.Ok()) {
    return MakeErrorResult<ProjectDocument>(err);
  }

  for (std::uint32_t i = 0; i < sectionCount; ++i) {
    auto sectionHeaderResult = ReadSectionHeader(reader);

    if (!sectionHeaderResult.Ok()) {
      return MakeErrorResult<ProjectDocument>(
        sectionHeaderResult.error
      );
    }

    const ProjectSectionHeader sectionHeader =
      sectionHeaderResult.value;

    auto payloadResult = ReadSectionPayload(
      reader,
      sectionHeader
    );

    if (!payloadResult.Ok()) {
      return MakeErrorResult<ProjectDocument>(
        payloadResult.error
      );
    }

    std::string sectionData(
      reinterpret_cast<const char*>(payloadResult.value.data()),
      payloadResult.value.size()
    );

    std::istringstream sectionStream(
      sectionData,
      std::ios::in | std::ios::binary
    );

    BinaryReader sectionReader =
      MakeReaderForPayload(sectionStream);

    switch (sectionHeader.id) {
      case ProjectSectionId::Tracks: {
        err = ReadTracksSection(sectionReader, project);
        break;
      }

      case ProjectSectionId::Patterns: {
        err = ReadPatternsSection(sectionReader, project);
        break;
      }

      case ProjectSectionId::Arrangement: {
        err = ReadArrangementSection(sectionReader, project);
        break;
      }

      case ProjectSectionId::Generators: {
        err = ReadGeneratorsSection(sectionReader, project);
        break;
      }

      default: {
        // Unknown project sections are ignored so newer files can carry
        // optional data without breaking older readers.
        err = Success();
        break;
      }
    }

    if (!err.Ok()) {
      return MakeErrorResult<ProjectDocument>(err);
    }
  }

  return Result<ProjectDocument> {
    project,
    Success()
  };
}

Error ProjectDocumentCodec::WriteTracksSection(
  BinaryWriter& writer,
  const ProjectDocument& project
) {
  auto payloadResult = BuildSectionPayload(
    [&project](BinaryWriter& sectionWriter) {
      auto err = sectionWriter.WriteU64(
        static_cast<std::uint64_t>(project.tracks.size())
      );
      if (!err.Ok()) return err;

      for (const auto& track : project.tracks) {
        err = sectionWriter.WriteU64(track.id);
        if (!err.Ok()) return err;

        err = sectionWriter.WriteString(track.name);
        if (!err.Ok()) return err;

        err = sectionWriter.WriteU64(
          static_cast<std::uint64_t>(track.generatorNodeIds.size())
        );
        if (!err.Ok()) return err;

        for (const auto generatorNodeId : track.generatorNodeIds) {
          err = sectionWriter.WriteU64(generatorNodeId);
          if (!err.Ok()) return err;
        }
      }

      return Success();
    }
  );

  if (!payloadResult.Ok()) {
    return payloadResult.error;
  }

  return WriteSection(
    writer,
    ProjectSectionId::Tracks,
    1,
    std::span<const std::uint8_t>(
      payloadResult.value.data(),
      payloadResult.value.size()
    )
  );
}

Error ProjectDocumentCodec::WritePatternsSection(
  BinaryWriter& writer,
  const ProjectDocument& project
) {
  auto payloadResult = BuildSectionPayload(
    [&project](BinaryWriter& sectionWriter) {
      auto err = sectionWriter.WriteU64(
        static_cast<std::uint64_t>(project.patterns.size())
      );
      if (!err.Ok()) return err;

      for (const auto& pattern : project.patterns) {
        err = sectionWriter.WriteU64(pattern.id);
        if (!err.Ok()) return err;

        err = sectionWriter.WriteString(pattern.name);
        if (!err.Ok()) return err;

        err = sectionWriter.WriteF64(pattern.lengthBeats);
        if (!err.Ok()) return err;

        err = sectionWriter.WriteU64(
          static_cast<std::uint64_t>(pattern.notes.size())
        );
        if (!err.Ok()) return err;

        for (const auto& note : pattern.notes) {
          err = sectionWriter.WriteU64(note.id);
          if (!err.Ok()) return err;

          err = sectionWriter.WriteU32(
            static_cast<std::uint32_t>(note.midiNote)
          );
          if (!err.Ok()) return err;

          err = sectionWriter.WriteF32(note.velocity);
          if (!err.Ok()) return err;

          err = sectionWriter.WriteF64(note.startBeat);
          if (!err.Ok()) return err;

          err = sectionWriter.WriteF64(note.lengthBeats);
          if (!err.Ok()) return err;
        }
      }

      return Success();
    }
  );

  if (!payloadResult.Ok()) {
    return payloadResult.error;
  }

  return WriteSection(
    writer,
    ProjectSectionId::Patterns,
    1,
    std::span<const std::uint8_t>(
      payloadResult.value.data(),
      payloadResult.value.size()
    )
  );
}

Error ProjectDocumentCodec::WriteArrangementSection(
  BinaryWriter& writer,
  const ProjectDocument& project
) {
  auto payloadResult = BuildSectionPayload(
    [&project](BinaryWriter& sectionWriter) {
      auto err = sectionWriter.WriteU64(
        static_cast<std::uint64_t>(
          project.arrangementClips.size()
        )
      );
      if (!err.Ok()) return err;

      for (const auto& clip : project.arrangementClips) {
        err = sectionWriter.WriteU64(clip.id);
        if (!err.Ok()) return err;

        err = sectionWriter.WriteU64(clip.trackId);
        if (!err.Ok()) return err;

        err = sectionWriter.WriteU64(clip.patternId);
        if (!err.Ok()) return err;

        err = sectionWriter.WriteF64(clip.startBeat);
        if (!err.Ok()) return err;

        err = sectionWriter.WriteF64(clip.lengthBeats);
        if (!err.Ok()) return err;

        err = sectionWriter.WriteF64(clip.patternStartBeat);
        if (!err.Ok()) return err;

        err = sectionWriter.WriteBool(clip.muted);
        if (!err.Ok()) return err;
      }

      return Success();
    }
  );

  if (!payloadResult.Ok()) {
    return payloadResult.error;
  }

  return WriteSection(
    writer,
    ProjectSectionId::Arrangement,
    1,
    std::span<const std::uint8_t>(
      payloadResult.value.data(),
      payloadResult.value.size()
    )
  );
}

Error ProjectDocumentCodec::ReadTracksSection(
  BinaryReader& reader,
  ProjectDocument& project
) {
  std::uint64_t count = 0;

  auto err = reader.ReadU64(count);
  if (!err.Ok()) return err;

  err = CheckCount(count, "Track");
  if (!err.Ok()) return err;

  project.tracks.clear();
  project.tracks.reserve(static_cast<std::size_t>(count));

  for (std::uint64_t i = 0; i < count; ++i) {
    ProjectTrack track;

    err = reader.ReadU64(track.id);
    if (!err.Ok()) return err;

    err = reader.ReadString(track.name);
    if (!err.Ok()) return err;

    std::uint64_t generatorNodeCount = 0;

    err = reader.ReadU64(generatorNodeCount);
    if (!err.Ok()) return err;

    err = CheckCount(generatorNodeCount, "Track generator node");
    if (!err.Ok()) return err;

    track.generatorNodeIds.clear();
    track.generatorNodeIds.reserve(
      static_cast<std::size_t>(generatorNodeCount)
    );

    for (std::uint64_t generatorIndex = 0;
         generatorIndex < generatorNodeCount;
         ++generatorIndex) {

      std::uint64_t generatorNodeId = 0;

      err = reader.ReadU64(generatorNodeId);
      if (!err.Ok()) return err;

      track.generatorNodeIds.push_back(generatorNodeId);
    }

    project.tracks.push_back(std::move(track));
  }

  return Success();
}

Error ProjectDocumentCodec::ReadPatternsSection(
  BinaryReader& reader,
  ProjectDocument& project
) {
  std::uint64_t patternCount = 0;

  auto err = reader.ReadU64(patternCount);
  if (!err.Ok()) return err;

  err = CheckCount(patternCount, "Pattern");
  if (!err.Ok()) return err;

  project.patterns.clear();
  project.patterns.reserve(static_cast<std::size_t>(patternCount));

  for (std::uint64_t i = 0; i < patternCount; ++i) {
    ProjectPattern pattern;

    err = reader.ReadU64(pattern.id);
    if (!err.Ok()) return err;

    err = reader.ReadString(pattern.name);
    if (!err.Ok()) return err;

    err = reader.ReadF64(pattern.lengthBeats);
    if (!err.Ok()) return err;

    std::uint64_t noteCount = 0;

    err = reader.ReadU64(noteCount);
    if (!err.Ok()) return err;

    err = CheckCount(noteCount, "Pattern note");
    if (!err.Ok()) return err;

    pattern.notes.clear();
    pattern.notes.reserve(static_cast<std::size_t>(noteCount));

    for (std::uint64_t noteIndex = 0; noteIndex < noteCount; ++noteIndex) {
      ProjectPatternNote note;

      err = reader.ReadU64(note.id);
      if (!err.Ok()) return err;

      std::uint32_t midiNote = 0;

      err = reader.ReadU32(midiNote);
      if (!err.Ok()) return err;

      note.midiNote = static_cast<std::int32_t>(midiNote);

      err = reader.ReadF32(note.velocity);
      if (!err.Ok()) return err;

      err = reader.ReadF64(note.startBeat);
      if (!err.Ok()) return err;

      err = reader.ReadF64(note.lengthBeats);
      if (!err.Ok()) return err;

      pattern.notes.push_back(std::move(note));
    }

    project.patterns.push_back(std::move(pattern));
  }

  return Success();
}

Error ProjectDocumentCodec::ReadArrangementSection(
  BinaryReader& reader,
  ProjectDocument& project
) {
  std::uint64_t count = 0;

  auto err = reader.ReadU64(count);
  if (!err.Ok()) return err;

  err = CheckCount(count, "Arrangement clip");
  if (!err.Ok()) return err;

  project.arrangementClips.clear();
  project.arrangementClips.reserve(static_cast<std::size_t>(count));

  for (std::uint64_t i = 0; i < count; ++i) {
    ProjectArrangementClip clip;

    err = reader.ReadU64(clip.id);
    if (!err.Ok()) return err;

    err = reader.ReadU64(clip.trackId);
    if (!err.Ok()) return err;

    err = reader.ReadU64(clip.patternId);
    if (!err.Ok()) return err;

    err = reader.ReadF64(clip.startBeat);
    if (!err.Ok()) return err;

    err = reader.ReadF64(clip.lengthBeats);
    if (!err.Ok()) return err;

    err = reader.ReadF64(clip.patternStartBeat);
    if (!err.Ok()) return err;

    err = reader.ReadBool(clip.muted);
    if (!err.Ok()) return err;

    project.arrangementClips.push_back(std::move(clip));
  }

  return Success();
}
}