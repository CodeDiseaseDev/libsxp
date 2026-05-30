#include "asset/AssetWriter.h"

#include "chunk/ChunkId.h"
#include "path/PathSanitizer.h"
#include "stream/BinaryWriter.h"

#include <fstream>
#include <iterator>
#include <sstream>
#include <vector>

namespace {
  std::vector<std::uint8_t> ToBytes(const std::string& value) {
    return std::vector<std::uint8_t>(
      value.begin(),
      value.end()
    );
  }
}

namespace sxp {
  static Error SerializeAssetManifest(
    const AssetManifest& manifest,
    std::vector<std::uint8_t>& out
  ) {
    std::ostringstream stream(std::ios::binary);
    BinaryWriter writer(stream);

    auto err = writer.WriteU64(
      static_cast<std::uint64_t>(manifest.assets.size())
    );
    if (!err.Ok()) return err;

    for (const auto& asset : manifest.assets) {
      err = writer.WriteU64(asset.id.high);
      if (!err.Ok()) return err;

      err = writer.WriteU64(asset.id.low);
      if (!err.Ok()) return err;

      err = writer.WriteU32(
        static_cast<std::uint32_t>(asset.kind)
      );
      if (!err.Ok()) return err;

      err = writer.WriteString(asset.displayName);
      if (!err.Ok()) return err;

      err = writer.WriteString(asset.originalPathHint);
      if (!err.Ok()) return err;

      err = writer.WriteString(asset.mimeType);
      if (!err.Ok()) return err;

      err = writer.WriteU64(asset.blobChunkIndex);
      if (!err.Ok()) return err;

      err = writer.WriteU64(asset.storedSize);
      if (!err.Ok()) return err;

      err = writer.WriteU64(asset.uncompressedSize);
      if (!err.Ok()) return err;

      err = writer.WriteU32(asset.flags);
      if (!err.Ok()) return err;

      err = writer.WriteU64(asset.checksum);
      if (!err.Ok()) return err;
    }

    out = ToBytes(stream.str());
    return Success();
  }

  Result<AssetId> AssetWriter::EmbedFile(
    FileWriter& writer,
    const std::string& path,
    AssetKind kind,
    const std::string& mimeType
  ) {
    std::ifstream file(path, std::ios::binary);

    if (!file.is_open()) {
      return {
        {},
        Fail(
          ErrorCode::FileOpenFailed,
          "Failed to open asset file for embedding."
        )
      };
    }

    // std::vector<std::uint8_t> bytes(
    //   std::istreambuf_iterator<char>(file),
    //   std::istreambuf_iterator<char>()
    // );

    std::vector<std::uint8_t> bytes;

    char byte = 0;
    while (file.get(byte)) {
      bytes.push_back(static_cast<std::uint8_t>(byte));
    }

    AssetId id;
    id.high = 0;
    id.low = nextAssetId_++;

    const std::uint64_t blobChunkIndex =
      writer.GetWrittenChunkCount();

    auto err = writer.WriteChunk(
      Chunk_BLOB,
      1,
      std::span<const std::uint8_t>(
        bytes.data(),
        bytes.size()
      )
    );

    if (!err.Ok()) {
      return {{}, err};
    }

    EmbeddedAsset asset;
    asset.id = id;
    asset.kind = kind;
    asset.displayName = PathSanitizer::MakeSafeDisplayName(path);
    asset.originalPathHint = path;
    asset.mimeType = mimeType;
    asset.blobChunkIndex = blobChunkIndex;
    asset.storedSize = bytes.size();
    asset.uncompressedSize = bytes.size();
    asset.flags = 0;
    asset.checksum = 0;

    manifest_.assets.push_back(std::move(asset));

    return {id, Success()};
  }

  Error AssetWriter::WriteManifest(FileWriter& writer) {
    std::vector<std::uint8_t> payload;

    auto err = SerializeAssetManifest(manifest_, payload);

    if (!err.Ok()) {
      return err;
    }

    return writer.WriteChunk(
      Chunk_ASST,
      1,
      std::span<const std::uint8_t>(
        payload.data(),
        payload.size()
      )
    );
  }

  const AssetManifest& AssetWriter::GetManifest() const {
    return manifest_;
  }
}