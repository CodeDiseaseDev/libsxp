//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_CHUNKID_H
#define LIBSXP_CHUNKID_H

#include <cstdint>

namespace sxp {
  using FourCC = std::uint32_t;
  using ChunkId = FourCC;

  constexpr FourCC MakeFourCC(char a, char b, char c, char d) {
    return
      static_cast<std::uint32_t>(a) |
      (static_cast<std::uint32_t>(b) << 8) |
      (static_cast<std::uint32_t>(c) << 16) |
      (static_cast<std::uint32_t>(d) << 24);
  }

  constexpr FourCC Chunk_META = MakeFourCC('M', 'E', 'T', 'A');
  constexpr FourCC Chunk_PROJ = MakeFourCC('P', 'R', 'O', 'J');
  constexpr FourCC Chunk_ASST = MakeFourCC('A', 'S', 'S', 'T');
  constexpr FourCC Chunk_BLOB = MakeFourCC('B', 'L', 'O', 'B');
  constexpr FourCC Chunk_TOC_ = MakeFourCC('T', 'O', 'C', ' ');
}

#endif // LIBSXP_CHUNKID_H
