//
// Created by code on 6/1/26.
//

#include <functional>
#include <sstream>
#include <sxp/project/generator_payload/GeneratorPayloadCodec.h>
#include <sxp/stream/BinaryWriter.h>

#include "sxp/project/generator_payload/PayloadReaderContext.h"
#include "sxp/stream/BinaryReader.h"

namespace {
  using PayloadWriteFunc = std::function<sxp::Error(sxp::BinaryWriter&)>;

  template<typename T>
  sxp::Result<T> MakeErrorResult(sxp::Error error) {
    sxp::Result<T> result;
    result.error = std::move(error);
    return result;
  }

  sxp::Result<std::vector<std::uint8_t>> BuildPayload(
    const PayloadWriteFunc& writePayload
  ) {
    std::ostringstream stream(
      std::ios::out | std::ios::binary
    );

    sxp::BinaryWriter writer(stream);

    auto err = writePayload(writer);

    if (!err.Ok()) {
      return MakeErrorResult<std::vector<std::uint8_t>>(err);
    }

    const std::string data = stream.str();

    std::vector<std::uint8_t> bytes(
      data.begin(),
      data.end()
    );

    return sxp::Result<std::vector<std::uint8_t>> {
      std::move(bytes),
      sxp::Success()
    };
  }
}

namespace sxp {
  Result<std::vector<std::uint8_t>>
  ProjectGeneratorPayloadCodec::EncodeOscillatorV1(
    const ProjectOscillatorPayloadV1& payload
  ) {
    return BuildPayload([&payload](BinaryWriter& writer) {
      auto err = writer.WriteU32(payload.waveShape);
      if (!err.Ok()) return err;

      err = writer.WriteI32(payload.octave);
      if (!err.Ok()) return err;

      err = writer.WriteI32(payload.semitone);
      if (!err.Ok()) return err;

      err = writer.WriteF32(payload.attackSeconds);
      if (!err.Ok()) return err;

      err = writer.WriteF32(payload.decaySeconds);
      if (!err.Ok()) return err;

      err = writer.WriteF32(payload.sustainLevel);
      if (!err.Ok()) return err;

      err = writer.WriteF32(payload.releaseSeconds);
      if (!err.Ok()) return err;

      err = writer.WriteU64(payload.mixerChannelId);
      if (!err.Ok()) return err;

      return Success();
    });
  }

  Result<ProjectOscillatorPayloadV1>
  ProjectGeneratorPayloadCodec::DecodeOscillatorV1(
    std::span<const std::uint8_t> bytes
  ) {
    PayloadReaderContext context(bytes);
    BinaryReader& reader = context.reader;

    ProjectOscillatorPayloadV1 payload;

    auto err = reader.ReadU32(payload.waveShape);
    if (!err.Ok()) return MakeErrorResult<ProjectOscillatorPayloadV1>(err);

    err = reader.ReadI32(payload.octave);
    if (!err.Ok()) return MakeErrorResult<ProjectOscillatorPayloadV1>(err);

    err = reader.ReadI32(payload.semitone);
    if (!err.Ok()) return MakeErrorResult<ProjectOscillatorPayloadV1>(err);

    err = reader.ReadF32(payload.attackSeconds);
    if (!err.Ok()) return MakeErrorResult<ProjectOscillatorPayloadV1>(err);

    err = reader.ReadF32(payload.decaySeconds);
    if (!err.Ok()) return MakeErrorResult<ProjectOscillatorPayloadV1>(err);

    err = reader.ReadF32(payload.sustainLevel);
    if (!err.Ok()) return MakeErrorResult<ProjectOscillatorPayloadV1>(err);

    err = reader.ReadF32(payload.releaseSeconds);
    if (!err.Ok()) return MakeErrorResult<ProjectOscillatorPayloadV1>(err);

    err = reader.ReadU64(payload.mixerChannelId);
    if (!err.Ok()) return MakeErrorResult<ProjectOscillatorPayloadV1>(err);

    return Result<ProjectOscillatorPayloadV1> {
      std::move(payload),
      Success()
    };
  }

  Result<std::vector<std::uint8_t>>
  ProjectGeneratorPayloadCodec::EncodeSamplerV1(
    const ProjectSamplerPayloadV1& payload
  ) {
    return BuildPayload([&payload](BinaryWriter& writer) {
      auto err = writer.WriteU64(payload.sampleAssetId.high);
      if (!err.Ok()) return err;

      err = writer.WriteU64(payload.sampleAssetId.low);
      if (!err.Ok()) return err;

      err = writer.WriteString(payload.samplePath);
      if (!err.Ok()) return err;

      err = writer.WriteI32(payload.rootNote);
      if (!err.Ok()) return err;

      err = writer.WriteBool(payload.pitchFollow);
      if (!err.Ok()) return err;

      err = writer.WriteBool(payload.oneShot);
      if (!err.Ok()) return err;

      err = writer.WriteU64(payload.mixerChannelId);
      if (!err.Ok()) return err;

      return Success();
    });
  }

  Result<ProjectSamplerPayloadV1>
  ProjectGeneratorPayloadCodec::DecodeSamplerV1(
    std::span<const std::uint8_t> bytes
  ) {
    PayloadReaderContext context(bytes);
    BinaryReader& reader = context.reader;

    ProjectSamplerPayloadV1 payload;

    auto err = reader.ReadU64(payload.sampleAssetId.high);
    if (!err.Ok()) return MakeErrorResult<ProjectSamplerPayloadV1>(err);

    err = reader.ReadU64(payload.sampleAssetId.low);
    if (!err.Ok()) return MakeErrorResult<ProjectSamplerPayloadV1>(err);

    err = reader.ReadString(payload.samplePath);
    if (!err.Ok()) return MakeErrorResult<ProjectSamplerPayloadV1>(err);

    err = reader.ReadI32(payload.rootNote);
    if (!err.Ok()) return MakeErrorResult<ProjectSamplerPayloadV1>(err);

    err = reader.ReadBool(payload.pitchFollow);
    if (!err.Ok()) return MakeErrorResult<ProjectSamplerPayloadV1>(err);

    err = reader.ReadBool(payload.oneShot);
    if (!err.Ok()) return MakeErrorResult<ProjectSamplerPayloadV1>(err);

    err = reader.ReadU64(payload.mixerChannelId);
    if (!err.Ok()) return MakeErrorResult<ProjectSamplerPayloadV1>(err);

    return Result<ProjectSamplerPayloadV1> {
      std::move(payload),
      Success()
    };
  }
}
