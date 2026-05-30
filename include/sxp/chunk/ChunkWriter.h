#pragma once

#include "sxp/chunk/ChunkEntry.h"
#include "sxp/chunk/ChunkHeader.h"
#include "sxp/core/Error.h"
#include "sxp/stream/BinaryWriter.h"

#include <cstdint>
#include <ostream>
#include <span>
#include <vector>

namespace sxp {
  class ChunkWriter {
  public:
    explicit ChunkWriter(std::ostream& stream);

    Error WriteChunk(
      FourCC id,
      std::uint32_t version,
      std::span<const std::uint8_t> payload
    );

    const std::vector<ChunkEntry>& GetChunks() const;

  private:
    Error WriteChunkHeader(const ChunkHeader& header);

    std::ostream& stream_;
    BinaryWriter writer_;
    std::vector<ChunkEntry> chunks_;
  };
}