//
// Created by code on 5/31/26.
//

#ifndef LIBSXP_SXPFORMAT_H
#define LIBSXP_SXPFORMAT_H

#include <cstdint>
#include <array>

namespace sxp {

  inline constexpr std::array<char, 4> Magic = { 'S', 'X', 'P', '\0' };

  inline constexpr std::uint16_t CurrentMajorVersion = 1;
  inline constexpr std::uint16_t CurrentMinorVersion = 0;

}

#endif //LIBSXP_SXPFORMAT_H
