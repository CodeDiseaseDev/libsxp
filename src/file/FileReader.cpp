#include "sxp/file/FileReader.h"

#include "sxp/chunk/ChunkId.h"
#include "sxp/chunk/ChunkReader.h"
#include "sxp/stream/BinaryReader.h"

#include <cstring>
#include <sstream>
#include "sxp/sxp_format/SxpFormat.h"

namespace sxp {
  static Result<TableOfContents> DeserializeTableOfContents(
    const std::vector<std::uint8_t>& bytes,
    ReadLimits limits
  ) {
    std::string data(
      reinterpret_cast<const char*>(bytes.data()),
      bytes.size()
    );

    std::istringstream stream(data, std::ios::binary);
    BinaryReader reader(stream, limits);

    std::uint64_t count = 0;

    auto err = reader.ReadU64(count);

    if (!err.Ok()) {
      return {{}, err};
    }

    if (count > limits.maxChunkCount) {
      return {
        {},
        Fail(
          ErrorCode::SizeLimitExceeded,
          "TOC chunk count exceeded size limit."
        )
      };
    }

    TableOfContents toc;
    toc.chunks.resize(static_cast<std::size_t>(count));

    for (auto& chunk : toc.chunks) {
      err = reader.ReadU32(chunk.id);
      if (!err.Ok()) return {{}, err};

      err = reader.ReadU32(chunk.version);
      if (!err.Ok()) return {{}, err};

      err = reader.ReadU32(chunk.flags);
      if (!err.Ok()) return {{}, err};

      err = reader.ReadU64(chunk.offset);
      if (!err.Ok()) return {{}, err};

      err = reader.ReadU64(chunk.storedSize);
      if (!err.Ok()) return {{}, err};

      err = reader.ReadU64(chunk.uncompressedSize);
      if (!err.Ok()) return {{}, err};

      err = reader.ReadU64(chunk.checksum);
      if (!err.Ok()) return {{}, err};

      if (chunk.storedSize > limits.maxChunkBytes ||
          chunk.uncompressedSize > limits.maxChunkBytes) {
        return {
          {},
          Fail(
            ErrorCode::SizeLimitExceeded,
            "TOC entry exceeded chunk size limit."
          )
        };
      }
    }

    return {toc, Success()};
  }

  Error FileReader::Open(
    const std::string& path,
    ReadLimits limits
  ) {
    limits_ = limits;

    file_.open(path, std::ios::binary);

    if (!file_.is_open()) {
      return Fail(
        ErrorCode::FileOpenFailed,
        "Failed to open SXP file for reading."
      );
    }

    file_.seekg(0, std::ios::end);

    const auto fileSize =
      static_cast<std::uint64_t>(file_.tellg());

    if (fileSize > limits_.maxFileBytes) {
      return Fail(
        ErrorCode::SizeLimitExceeded,
        "SXP file exceeded max file size."
      );
    }

    file_.seekg(0, std::ios::beg);

    auto headerResult = ReadHeader();

    if (!headerResult.Ok()) {
      return headerResult.error;
    }

    header_ = headerResult.value;

    auto err = ValidateHeader(header_);

    if (!err.Ok()) {
      return err;
    }

    err = ReadTableOfContents();

    if (!err.Ok()) {
      return err;
    }

    opened_ = true;
    return Success();
  }

  const Header& FileReader::GetHeader() const {
    return header_;
  }

  const TableOfContents& FileReader::GetTableOfContents() const {
    return tableOfContents_;
  }

  Result<std::vector<std::uint8_t>> FileReader::ReadChunkPayload(
    const ChunkEntry& entry
  ) {
    if (!opened_) {
      return {
        {},
        Fail(
          ErrorCode::FileReadFailed,
          "FileReader is not open."
        )
      };
    }

    if (entry.storedSize > limits_.maxChunkBytes) {
      return {
        {},
        Fail(
          ErrorCode::SizeLimitExceeded,
          "Chunk exceeded size limit."
        )
      };
    }

    file_.seekg(
      static_cast<std::streamoff>(entry.offset),
      std::ios::beg
    );

    if (!file_) {
      return {
        {},
        Fail(
          ErrorCode::FileReadFailed,
          "Failed to seek to chunk."
        )
      };
    }

    ChunkReader chunkReader(file_, limits_);
    auto headerResult = chunkReader.ReadChunkHeader();

    if (!headerResult.Ok()) {
      return {{}, headerResult.error};
    }

    const ChunkHeader& header = headerResult.value;

    if (header.id != entry.id ||
        header.version != entry.version ||
        header.storedSize != entry.storedSize ||
        header.uncompressedSize != entry.uncompressedSize) {
      return {
        {},
        Fail(
          ErrorCode::InvalidChunk,
          "Chunk header did not match TOC entry."
        )
      };
    }

    return chunkReader.ReadPayload(header);
  }

  Result<Header> FileReader::ReadHeader() {
    Header header;
    BinaryReader reader(file_, limits_);

    auto err = reader.ReadBytes(
      std::span<std::uint8_t>(
        reinterpret_cast<std::uint8_t*>(header.magic),
        4
      )
    );
    if (!err.Ok()) return {{}, err};

    err = reader.ReadU16(header.majorVersion);
    if (!err.Ok()) return {{}, err};

    err = reader.ReadU16(header.minorVersion);
    if (!err.Ok()) return {{}, err};

    err = reader.ReadU32(header.flags);
    if (!err.Ok()) return {{}, err};

    err = reader.ReadU64(header.tocOffset);
    if (!err.Ok()) return {{}, err};

    err = reader.ReadU64(header.chunkCount);
    if (!err.Ok()) return {{}, err};

    return {header, Success()};
  }

  Error FileReader::ValidateHeader(const Header& header) {
    if (std::memcmp(header.magic, sxp::Magic.data(), sxp::Magic.size()) != 0) {
      return Fail(
        ErrorCode::InvalidMagic,
        "Invalid SXP magic."
      );
    }

    if (header.majorVersion != sxp::CurrentMajorVersion) {
      return Fail(
        ErrorCode::UnsupportedVersion,
        "Unsupported SXP major version."
      );
    }

    if (header.minorVersion != sxp::CurrentMinorVersion) {
      // optional warning (not important now)
    }

    if (header.chunkCount > limits_.maxChunkCount) {
      return Fail(
        ErrorCode::SizeLimitExceeded,
        "Chunk count exceeded size limit."
      );
    }

    return Success();
  }

  Error FileReader::ReadTableOfContents() {
    if (header_.tocOffset == 0) {
      return Fail(
        ErrorCode::MissingRequiredChunk,
        "Missing table of contents offset."
      );
    }

    file_.seekg(
      static_cast<std::streamoff>(header_.tocOffset),
      std::ios::beg
    );

    if (!file_) {
      return Fail(
        ErrorCode::FileReadFailed,
        "Failed to seek to table of contents."
      );
    }

    ChunkReader chunkReader(file_, limits_);
    auto tocHeaderResult = chunkReader.ReadChunkHeader();

    if (!tocHeaderResult.Ok()) {
      return tocHeaderResult.error;
    }

    const ChunkHeader& tocHeader = tocHeaderResult.value;

    if (tocHeader.id != Chunk_TOC_) {
      return Fail(
        ErrorCode::MissingRequiredChunk,
        "Expected TOC chunk at header TOC offset."
      );
    }

    auto payloadResult = chunkReader.ReadPayload(tocHeader);

    if (!payloadResult.Ok()) {
      return payloadResult.error;
    }

    auto tocResult = DeserializeTableOfContents(
      payloadResult.value,
      limits_
    );

    if (!tocResult.Ok()) {
      return tocResult.error;
    }

    if (tocResult.value.chunks.size() != header_.chunkCount) {
      return Fail(
        ErrorCode::InvalidChunk,
        "TOC chunk count did not match header chunk count."
      );
    }

    tableOfContents_ = std::move(tocResult.value);
    return Success();
  }
}