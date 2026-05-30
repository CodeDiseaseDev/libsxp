#include "sxp/stream/BinaryWriter.h"

#include <cstring>
#include <limits>

namespace sxp {
  BinaryWriter::BinaryWriter(std::ostream& stream)
    : stream_(stream) {
  }

  Error BinaryWriter::WriteBytes(std::span<const std::uint8_t> bytes) {
    stream_.write(
      reinterpret_cast<const char*>(bytes.data()),
      static_cast<std::streamsize>(bytes.size())
    );

    if (!stream_) {
      return Fail(ErrorCode::FileWriteFailed, "Failed to write bytes.");
    }

    return Success();
  }

  Error BinaryWriter::WriteU8(std::uint8_t value) {
    const std::uint8_t bytes[1] = { value };
    return WriteBytes(bytes);
  }

  Error BinaryWriter::WriteU16(std::uint16_t value) {
    const std::uint8_t bytes[2] = {
      static_cast<std::uint8_t>(value & 0xFF),
      static_cast<std::uint8_t>((value >> 8) & 0xFF)
    };

    return WriteBytes(bytes);
  }

  Error BinaryWriter::WriteU32(std::uint32_t value) {
    const std::uint8_t bytes[4] = {
      static_cast<std::uint8_t>(value & 0xFF),
      static_cast<std::uint8_t>((value >> 8) & 0xFF),
      static_cast<std::uint8_t>((value >> 16) & 0xFF),
      static_cast<std::uint8_t>((value >> 24) & 0xFF)
    };

    return WriteBytes(bytes);
  }

  Error BinaryWriter::WriteU64(std::uint64_t value) {
    const std::uint8_t bytes[8] = {
      static_cast<std::uint8_t>(value & 0xFF),
      static_cast<std::uint8_t>((value >> 8) & 0xFF),
      static_cast<std::uint8_t>((value >> 16) & 0xFF),
      static_cast<std::uint8_t>((value >> 24) & 0xFF),
      static_cast<std::uint8_t>((value >> 32) & 0xFF),
      static_cast<std::uint8_t>((value >> 40) & 0xFF),
      static_cast<std::uint8_t>((value >> 48) & 0xFF),
      static_cast<std::uint8_t>((value >> 56) & 0xFF)
    };

    return WriteBytes(bytes);
  }

  Error BinaryWriter::WriteF32(float value) {
    std::uint32_t bits = 0;
    static_assert(sizeof(bits) == sizeof(value));

    std::memcpy(&bits, &value, sizeof(bits));
    return WriteU32(bits);
  }

  Error BinaryWriter::WriteF64(double value) {
    std::uint64_t bits = 0;
    static_assert(sizeof(bits) == sizeof(value));

    std::memcpy(&bits, &value, sizeof(bits));
    return WriteU64(bits);
  }

  Error BinaryWriter::WriteBool(bool value) {
    std::uint8_t byte = static_cast<std::uint8_t>(value ? 1 : 0);

    return WriteU8(byte);
  }

  Error BinaryWriter::WriteString(const std::string& value) {
    if (value.size() > std::numeric_limits<std::uint32_t>::max()) {
      return Fail(ErrorCode::InvalidSize, "String is too large.");
    }

    auto err = WriteU32(static_cast<std::uint32_t>(value.size()));

    if (!err.Ok()) {
      return err;
    }

    if (value.empty()) {
      return Success();
    }

    return WriteBytes(
      std::span<const std::uint8_t>(
        reinterpret_cast<const std::uint8_t*>(value.data()),
        value.size()
      )
    );
  }

  Error BinaryWriter::WriteI32(std::int32_t value) {
    const auto raw = std::bit_cast<std::uint32_t>(value);

    std::uint8_t bytes[4] {
      static_cast<std::uint8_t>(raw & 0xFF),
      static_cast<std::uint8_t>((raw >> 8) & 0xFF),
      static_cast<std::uint8_t>((raw >> 16) & 0xFF),
      static_cast<std::uint8_t>((raw >> 24) & 0xFF)
    };

    return WriteBytes(bytes);
  }
}
