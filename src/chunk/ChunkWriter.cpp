#include "chunk/ChunkWriter.h"

namespace sxp {
  ChunkWriter::ChunkWriter(std::ostream& stream)
    : stream_(stream),
      writer_(stream) {
  }

  Error ChunkWriter::WriteChunk(
    FourCC id,
    std::uint32_t version,
    std::span<const std::uint8_t> payload
  ) {
    const std::uint64_t offset =
      static_cast<std::uint64_t>(stream_.tellp());

    ChunkHeader header;
    header.id = id;
    header.version = version;
    header.flags = 0;
    header.storedSize = payload.size();
    header.uncompressedSize = payload.size();
    header.checksum = 0;

    auto err = WriteChunkHeader(header);

    if (!err.Ok()) {
      return err;
    }

    err = writer_.WriteBytes(payload);

    if (!err.Ok()) {
      return err;
    }

    ChunkEntry entry;
    entry.id = header.id;
    entry.version = header.version;
    entry.flags = header.flags;
    entry.offset = offset;
    entry.storedSize = header.storedSize;
    entry.uncompressedSize = header.uncompressedSize;
    entry.checksum = header.checksum;

    chunks_.push_back(entry);

    return Success();
  }

  const std::vector<ChunkEntry>& ChunkWriter::GetChunks() const {
    return chunks_;
  }

  Error ChunkWriter::WriteChunkHeader(const ChunkHeader& header) {
    auto err = writer_.WriteU32(header.id);
    if (!err.Ok()) return err;

    err = writer_.WriteU32(header.version);
    if (!err.Ok()) return err;

    err = writer_.WriteU32(header.flags);
    if (!err.Ok()) return err;

    err = writer_.WriteU64(header.storedSize);
    if (!err.Ok()) return err;

    err = writer_.WriteU64(header.uncompressedSize);
    if (!err.Ok()) return err;

    err = writer_.WriteU64(header.checksum);
    if (!err.Ok()) return err;

    return Success();
  }
}