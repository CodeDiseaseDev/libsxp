#include "sxp/file/FileWriter.h"

#include <cstring>

#include "sxp/stream/BinaryWriter.h"

#include <sstream>
#include <vector>

#include "sxp/sxp_format/SxpFormat.h"

namespace {
  std::vector<std::uint8_t> ToBytes(const std::string& value) {
    return std::vector<std::uint8_t>(
      value.begin(),
      value.end()
    );
  }
}

namespace sxp {
  static Header CreateInitialHeader() {
    Header header {};

    std::memcpy(
      header.magic,
      Magic.data(),
      Magic.size()
    );

    header.majorVersion = CurrentMajorVersion;
    header.minorVersion = CurrentMinorVersion;

    header.flags = 0;
    header.tocOffset = 0;
    header.chunkCount = 0;

    return header;
  }
}

namespace sxp {
  static Error WriteHeader(BinaryWriter& writer, const Header& header) {
    auto err = writer.WriteBytes(
      std::span<const std::uint8_t>(
        reinterpret_cast<const std::uint8_t*>(header.magic),
        4
      )
    );
    if (!err.Ok()) return err;

    err = writer.WriteU16(header.majorVersion);
    if (!err.Ok()) return err;

    err = writer.WriteU16(header.minorVersion);
    if (!err.Ok()) return err;

    err = writer.WriteU32(header.flags);
    if (!err.Ok()) return err;

    err = writer.WriteU64(header.tocOffset);
    if (!err.Ok()) return err;

    err = writer.WriteU64(header.chunkCount);
    if (!err.Ok()) return err;

    return Success();
  }

  static Error SerializeTableOfContents(
    const std::vector<ChunkEntry>& chunks,
    std::vector<std::uint8_t>& out
  ) {
    std::ostringstream stream(std::ios::binary);
    BinaryWriter writer(stream);

    auto err = writer.WriteU64(
      static_cast<std::uint64_t>(chunks.size())
    );
    if (!err.Ok()) return err;

    for (const auto& chunk : chunks) {
      err = writer.WriteU32(chunk.id);
      if (!err.Ok()) return err;

      err = writer.WriteU32(chunk.version);
      if (!err.Ok()) return err;

      err = writer.WriteU32(chunk.flags);
      if (!err.Ok()) return err;

      err = writer.WriteU64(chunk.offset);
      if (!err.Ok()) return err;

      err = writer.WriteU64(chunk.storedSize);
      if (!err.Ok()) return err;

      err = writer.WriteU64(chunk.uncompressedSize);
      if (!err.Ok()) return err;

      err = writer.WriteU64(chunk.checksum);
      if (!err.Ok()) return err;
    }

    out = ToBytes(stream.str());
    return Success();
  }

  Error FileWriter::Open(const std::string& path) {
    if (opened_) {
      return Fail(
        ErrorCode::InvalidChunk,
        "FileWriter is already open."
      );
    }

    file_.open(path, std::ios::binary | std::ios::trunc);

    if (!file_.is_open()) {
      return Fail(
        ErrorCode::FileOpenFailed,
        "Failed to open SXP file for writing."
      );
    }

    header_ = CreateInitialHeader();
    opened_ = true;
    finalized_ = false;

    auto err = WriteHeaderAtStart();

    if (!err.Ok()) {
      return err;
    }

    chunkWriter_ = std::make_unique<ChunkWriter>(file_);

    return Success();
  }

  Error FileWriter::WriteChunk(
    ChunkId id,
    std::uint32_t version,
    std::span<const std::uint8_t> payload
  ) {
    if (!opened_ || finalized_ || chunkWriter_ == nullptr) {
      return Fail(
        ErrorCode::FileWriteFailed,
        "FileWriter is not open for chunk writing."
      );
    }

    return chunkWriter_->WriteChunk(id, version, payload);
  }

  Error FileWriter::Finalize() {
    if (!opened_) {
      return Fail(
        ErrorCode::FileWriteFailed,
        "FileWriter is not open."
      );
    }

    if (finalized_) {
      return Success();
    }

    auto err = WriteTableOfContents();

    if (!err.Ok()) {
      return err;
    }

    err = PatchHeaderAtStart();

    if (!err.Ok()) {
      return err;
    }

    file_.flush();

    if (!file_) {
      return Fail(
        ErrorCode::FileWriteFailed,
        "Failed to flush SXP file."
      );
    }

    finalized_ = true;
    file_.close();

    return Success();
  }

  std::uint64_t FileWriter::GetWrittenChunkCount() const {
    if (chunkWriter_ == nullptr) {
      return 0;
    }

    return static_cast<std::uint64_t>(
      chunkWriter_->GetChunks().size()
    );
  }

  Error FileWriter::WriteHeaderAtStart() {
    file_.seekp(0, std::ios::beg);

    if (!file_) {
      return Fail(
        ErrorCode::FileWriteFailed,
        "Failed to seek to SXP header."
      );
    }

    BinaryWriter writer(file_);
    return WriteHeader(writer, header_);
  }

  Error FileWriter::PatchHeaderAtStart() {
    return WriteHeaderAtStart();
  }

  Error FileWriter::WriteTableOfContents() {
    if (chunkWriter_ == nullptr) {
      return Fail(
        ErrorCode::FileWriteFailed,
        "Missing ChunkWriter."
      );
    }

    const auto chunksBeforeToc = chunkWriter_->GetChunks();

    std::vector<std::uint8_t> tocPayload;
    auto err = SerializeTableOfContents(
      chunksBeforeToc,
      tocPayload
    );

    if (!err.Ok()) {
      return err;
    }

    header_.tocOffset =
      static_cast<std::uint64_t>(file_.tellp());

    header_.chunkCount =
      static_cast<std::uint64_t>(chunksBeforeToc.size());

    err = chunkWriter_->WriteChunk(
      Chunk_TOC_,
      1,
      std::span<const std::uint8_t>(
        tocPayload.data(),
        tocPayload.size()
      )
    );

    if (!err.Ok()) {
      return err;
    }

    return Success();
  }
}