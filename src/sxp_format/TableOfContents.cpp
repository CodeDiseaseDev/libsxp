#include "sxp/sxp_format/TableOfContents.h"

namespace sxp {
  const ChunkEntry* TableOfContents::FindFirst(
    ChunkId id
  ) const {
    for (const auto& chunk : chunks) {
      if (chunk.id == id) {
        return &chunk;
      }
    }

    return nullptr;
  }

  std::vector<const ChunkEntry*> TableOfContents::FindAll(
    ChunkId id
  ) const {
    std::vector<const ChunkEntry*> matches;

    for (const auto& chunk : chunks) {
      if (chunk.id == id) {
        matches.push_back(&chunk);
      }
    }

    return matches;
  }
}