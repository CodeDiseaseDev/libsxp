#include "sxp/asset/AssetReader.h"

#include "sxp/chunk/ChunkId.h"
#include "sxp/path/PathSanitizer.h"

#include <fstream>

namespace sxp {
  AssetReader::AssetReader(
    FileReader& reader,
    AssetManifest manifest
  )
    : reader_(reader),
      manifest_(std::move(manifest)) {
  }

  Result<std::vector<std::uint8_t>> AssetReader::ReadAssetBytes(
    const AssetId& id
  ) {
    const EmbeddedAsset* asset = manifest_.FindById(id);

    if (asset == nullptr) {
      return {
        {},
        Fail(ErrorCode::AssetNotFound, "Asset was not found.")
      };
    }

    const auto& chunks =
      reader_.GetTableOfContents().chunks;

    if (asset->blobChunkIndex >= chunks.size()) {
      return {
        {},
        Fail(
          ErrorCode::InvalidChunk,
          "Asset blob chunk index was out of range."
        )
      };
    }

    const ChunkEntry& entry =
      chunks[static_cast<std::size_t>(asset->blobChunkIndex)];

    if (entry.id != Chunk_BLOB) {
      return {
        {},
        Fail(
          ErrorCode::InvalidChunk,
          "Asset chunk was not a BLOB chunk."
        )
      };
    }

    return reader_.ReadChunkPayload(entry);
  }

  Error AssetReader::ExtractAssetToFile(
    const AssetId& id,
    const std::string& safeOutputPath
  ) {
    if (!PathSanitizer::IsSafeRelativePath(safeOutputPath)) {
      return Fail(
        ErrorCode::InvalidSize,
        "Output path is not a safe relative path."
      );
    }

    auto bytesResult = ReadAssetBytes(id);

    if (!bytesResult.Ok()) {
      return bytesResult.error;
    }

    std::ofstream file(
      safeOutputPath,
      std::ios::binary | std::ios::trunc
    );

    if (!file.is_open()) {
      return Fail(
        ErrorCode::FileOpenFailed,
        "Failed to open output file for asset extraction."
      );
    }

    const auto& bytes = bytesResult.value;

    file.write(
      reinterpret_cast<const char*>(bytes.data()),
      static_cast<std::streamsize>(bytes.size())
    );

    if (!file) {
      return Fail(
        ErrorCode::FileWriteFailed,
        "Failed to write extracted asset file."
      );
    }

    return Success();
  }
}