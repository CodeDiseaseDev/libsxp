#pragma once

#include "core/Error.h"

#include <cstdint>
#include <ostream>
#include <span>
#include <string>
#include <vector>

namespace sxp {

class BinaryWriter {
public:
  explicit BinaryWriter(std::ostream& stream);

  Error WriteBytes(std::span<const std::uint8_t> bytes);
  Error WriteU8(std::uint8_t value);
  Error WriteU16(std::uint16_t value);
  Error WriteU32(std::uint32_t value);
  Error WriteU64(std::uint64_t value);

  Error WriteF32(float value);
  Error WriteF64(double value);

  Error WriteBool(bool value);

  Error WriteString(const std::string& value);

private:
  std::ostream& stream_;
};

}