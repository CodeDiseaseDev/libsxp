//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_FILEWRITER_H
#define LIBSXP_FILEWRITER_H

#include "sxp/chunk/ChunkId.h"
#include "sxp/chunk/ChunkWriter.h"
#include "sxp/core/Error.h"
#include "sxp/sxp_format/Header.h"

#include <cstdint>
#include <fstream>
#include <memory>
#include <span>
#include <string>

namespace sxp {
  class FileWriter {
  public:
    Error Open(const std::string& path);

    Error WriteChunk(
      ChunkId id,
      std::uint32_t version,
      std::span<const std::uint8_t> payload
    );

    Error Finalize();

    std::uint64_t GetWrittenChunkCount() const;

  private:
    Error WriteHeaderAtStart();
    Error PatchHeaderAtStart();
    Error WriteTableOfContents();

    std::ofstream file_;
    std::unique_ptr<ChunkWriter> chunkWriter_;
    Header header_;
    bool opened_ = false;
    bool finalized_ = false;
  };
}

#endif // LIBSXP_FILEWRITER_H