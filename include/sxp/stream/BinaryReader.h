#pragma once

#include "sxp/core/Error.h"
#include "sxp/stream/ReadLimits.h"

#include <cstdint>
#include <istream>
#include <span>
#include <string>

namespace sxp {
  class BinaryReader {
  public:
    explicit BinaryReader(
      std::istream& stream,
      ReadLimits limits = {}
    );

    Error ReadBytes(std::span<std::uint8_t> bytes);
    Error ReadU8(std::uint8_t& out);
    Error ReadU16(std::uint16_t& out);
    Error ReadU32(std::uint32_t& out);
    Error ReadU64(std::uint64_t& out);

    Error ReadF32(float& out);
    Error ReadF64(double& out);

    Error ReadBool(bool& out);

    Error ReadString(std::string& out);

  private:
    std::istream& stream_;
    ReadLimits limits_;
  };
}