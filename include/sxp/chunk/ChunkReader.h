//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_CHUNKREADER_H
#define LIBSXP_CHUNKREADER_H

#include "sxp/chunk/ChunkHeader.h"
#include "sxp/core/Result.h"
#include "sxp/stream/BinaryReader.h"

#include <cstdint>
#include <istream>
#include <vector>

namespace sxp {
  class ChunkReader {
  public:
    explicit ChunkReader(
      std::istream& stream,
      ReadLimits limits = {}
    );

    Result<ChunkHeader> ReadChunkHeader();
    Result<std::vector<std::uint8_t>> ReadPayload(
      const ChunkHeader& header
    );

  private:
    std::istream& stream_;
    BinaryReader reader_;
    ReadLimits limits_;
  };
}

#endif // LIBSXP_CHUNKREADER_H