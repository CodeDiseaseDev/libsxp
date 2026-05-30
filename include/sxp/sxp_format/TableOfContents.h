//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_TABLEOFCONTENTS_H
#define LIBSXP_TABLEOFCONTENTS_H
#include <vector>

#include "sxp/chunk/ChunkEntry.h"

namespace sxp {
  struct TableOfContents {
    std::vector<ChunkEntry> chunks;

    const ChunkEntry* FindFirst(ChunkId id) const;
    std::vector<const ChunkEntry*> FindAll(ChunkId id) const;
  };
}

#endif //LIBSXP_TABLEOFCONTENTS_H
