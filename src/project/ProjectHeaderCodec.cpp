#include "project/ProjectHeaderCodec.h"

namespace sxp {
  Error ProjectHeaderCodec::Write(
    BinaryWriter& writer,
    const ProjectHeader& header
  ) {
    // auto err = writer.WriteU32(header.projectFormatVersion);
    // if (!err.Ok()) return err;

    auto err = writer.WriteU64(header.projectIdHigh);
    if (!err.Ok()) return err;

    err = writer.WriteU64(header.projectIdLow);
    if (!err.Ok()) return err;

    err = writer.WriteString(header.projectName);
    if (!err.Ok()) return err;

    err = writer.WriteF64(header.bpm);
    if (!err.Ok()) return err;

    err = writer.WriteU32(header.timeSigNumerator);
    if (!err.Ok()) return err;

    err = writer.WriteU32(header.timeSigDenominator);
    if (!err.Ok()) return err;

    err = writer.WriteF64(header.swingAmount);
    if (!err.Ok()) return err;

    err = writer.WriteF64(header.swingSubdivisionBeats);
    if (!err.Ok()) return err;

    return Success();
  }

  Result<ProjectHeader> ProjectHeaderCodec::Read(
    BinaryReader& reader
  ) {
    ProjectHeader header;
    //
    // auto err = reader.ReadU32(header.projectFormatVersion);
    // if (!err.Ok()) return Result<ProjectHeader> { {}, err };

    auto err = reader.ReadU64(header.projectIdHigh);
    if (!err.Ok()) return Result<ProjectHeader> { {}, err };

    err = reader.ReadU64(header.projectIdLow);
    if (!err.Ok()) return Result<ProjectHeader> { {}, err };

    err = reader.ReadString(header.projectName);
    if (!err.Ok()) return Result<ProjectHeader> { {}, err };

    err = reader.ReadF64(header.bpm);
    if (!err.Ok()) return Result<ProjectHeader> { {}, err };

    err = reader.ReadU32(header.timeSigNumerator);
    if (!err.Ok()) return Result<ProjectHeader> { {}, err };

    err = reader.ReadU32(header.timeSigDenominator);
    if (!err.Ok()) return Result<ProjectHeader> { {}, err };

    err = reader.ReadF64(header.swingAmount);
    if (!err.Ok()) return Result<ProjectHeader> { {}, err };

    err = reader.ReadF64(header.swingSubdivisionBeats);
    if (!err.Ok()) return Result<ProjectHeader> { {}, err };

    return Result<ProjectHeader> {
      header,
      Success()
    };
  }
}