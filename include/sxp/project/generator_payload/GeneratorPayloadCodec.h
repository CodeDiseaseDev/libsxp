//
// Created by code on 6/1/26.
//

#ifndef LIBSXP_GENERATORPAYLOADCODEC_H
#define LIBSXP_GENERATORPAYLOADCODEC_H
#include <cstdint>
#include <span>
#include <vector>

#include "OscillatorPayload.h"
#include "SamplerPayload.h"
#include "sxp/core/Result.h"

namespace sxp {
  class ProjectGeneratorPayloadCodec {
  public:
    static Result<std::vector<std::uint8_t>> EncodeOscillatorV1(
      const ProjectOscillatorPayloadV1& payload
    );

    static Result<ProjectOscillatorPayloadV1> DecodeOscillatorV1(
      std::span<const std::uint8_t> bytes
    );

    static Result<std::vector<std::uint8_t>> EncodeSamplerV1(
      const ProjectSamplerPayloadV1& payload
    );

    static Result<ProjectSamplerPayloadV1> DecodeSamplerV1(
      std::span<const std::uint8_t> bytes
    );
  };
}

#endif //LIBSXP_GENERATORPAYLOADCODEC_H
