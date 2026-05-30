//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_FILEREADER_H
#define LIBSXP_FILEREADER_H

#include "chunk/ChunkEntry.h"
#include "core/Error.h"
#include "core/Result.h"
#include "stream/ReadLimits.h"
#include "sxp_format/Header.h"
#include "sxp_format/TableOfContents.h"

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace sxp {
  class FileReader {
  public:
    Error Open(const std::string& path, ReadLimits limits = {});

    const Header& GetHeader() const;
    const TableOfContents& GetTableOfContents() const;

    Result<std::vector<std::uint8_t>> ReadChunkPayload(
      const ChunkEntry& entry
    );

  private:
    Result<Header> ReadHeader();
    Error ReadTableOfContents();
    Error ValidateHeader(const Header& header);

    std::ifstream file_;
    Header header_;
    TableOfContents tableOfContents_;
    ReadLimits limits_;
    bool opened_ = false;
  };
}

#endif // LIBSXP_FILEREADER_H