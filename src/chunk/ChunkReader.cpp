#include "chunk/ChunkReader.h"

namespace sxp {
  ChunkReader::ChunkReader(
    std::istream& stream,
    ReadLimits limits
  )
    : stream_(stream),
      reader_(stream, limits),
      limits_(limits) {
  }

  Result<ChunkHeader> ChunkReader::ReadChunkHeader() {
    ChunkHeader header;

    auto err = reader_.ReadU32(header.id);
    if (!err.Ok()) return {{}, err};

    err = reader_.ReadU32(header.version);
    if (!err.Ok()) return {{}, err};

    err = reader_.ReadU32(header.flags);
    if (!err.Ok()) return {{}, err};

    err = reader_.ReadU64(header.storedSize);
    if (!err.Ok()) return {{}, err};

    err = reader_.ReadU64(header.uncompressedSize);
    if (!err.Ok()) return {{}, err};

    err = reader_.ReadU64(header.checksum);
    if (!err.Ok()) return {{}, err};

    if (header.storedSize > limits_.maxChunkBytes ||
        header.uncompressedSize > limits_.maxChunkBytes) {
      return {
          {},
          Fail(ErrorCode::SizeLimitExceeded, "Chunk exceeded size limit.")
        };
        }

    return {header, Success()};
  }

  Result<std::vector<std::uint8_t>> ChunkReader::ReadPayload(
    const ChunkHeader& header
  ) {
    if (header.storedSize > limits_.maxChunkBytes) {
      return {
          {},
          Fail(ErrorCode::SizeLimitExceeded, "Chunk exceeded size limit.")
        };
    }

    std::vector<std::uint8_t> payload;
    payload.resize(static_cast<std::size_t>(header.storedSize));

    if (payload.empty()) {
      return {payload, Success()};
    }

    auto err = reader_.ReadBytes(
      std::span<std::uint8_t>(
        payload.data(),
        payload.size()
      )
    );

    if (!err.Ok()) {
      return {{}, err};
    }

    return {payload, Success()};
  }
}