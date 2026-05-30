#include "sxp/stream/BinaryReader.h"

#include <cstring>

namespace sxp {
  BinaryReader::BinaryReader(
    std::istream& stream,
    ReadLimits limits
  )
    : stream_(stream),
      limits_(limits) {
  }

  Error BinaryReader::ReadBytes(std::span<std::uint8_t> bytes) {
    stream_.read(
      reinterpret_cast<char*>(bytes.data()),
      static_cast<std::streamsize>(bytes.size())
    );

    if (!stream_) {
      return Fail(ErrorCode::FileReadFailed, "Failed to read bytes.");
    }

    return Success();
  }

  Error BinaryReader::ReadU8(std::uint8_t& out) {
    std::uint8_t bytes[1] {};
    auto err = ReadBytes(bytes);

    if (!err.Ok()) {
      return err;
    }

    out = bytes[0];
    return Success();
  }

  Error BinaryReader::ReadU16(std::uint16_t& out) {
    std::uint8_t bytes[2] {};
    auto err = ReadBytes(bytes);

    if (!err.Ok()) {
      return err;
    }

    out =
      static_cast<std::uint16_t>(bytes[0]) |
      static_cast<std::uint16_t>(bytes[1] << 8);

    return Success();
  }

  Error BinaryReader::ReadU32(std::uint32_t& out) {
    std::uint8_t bytes[4] {};
    auto err = ReadBytes(bytes);

    if (!err.Ok()) {
      return err;
    }

    out =
      static_cast<std::uint32_t>(bytes[0]) |
      (static_cast<std::uint32_t>(bytes[1]) << 8) |
      (static_cast<std::uint32_t>(bytes[2]) << 16) |
      (static_cast<std::uint32_t>(bytes[3]) << 24);

    return Success();
  }

  Error BinaryReader::ReadU64(std::uint64_t& out) {
    std::uint8_t bytes[8] {};
    auto err = ReadBytes(bytes);

    if (!err.Ok()) {
      return err;
    }

    out =
      static_cast<std::uint64_t>(bytes[0]) |
      (static_cast<std::uint64_t>(bytes[1]) << 8) |
      (static_cast<std::uint64_t>(bytes[2]) << 16) |
      (static_cast<std::uint64_t>(bytes[3]) << 24) |
      (static_cast<std::uint64_t>(bytes[4]) << 32) |
      (static_cast<std::uint64_t>(bytes[5]) << 40) |
      (static_cast<std::uint64_t>(bytes[6]) << 48) |
      (static_cast<std::uint64_t>(bytes[7]) << 56);

    return Success();
  }

  Error BinaryReader::ReadF32(float& out) {
    std::uint32_t bits = 0;

    auto err = ReadU32(bits);

    if (!err.Ok()) {
      return err;
    }

    static_assert(sizeof(bits) == sizeof(out));
    std::memcpy(&out, &bits, sizeof(out));

    return Success();
  }

  Error BinaryReader::ReadF64(double& out) {
    std::uint64_t bits = 0;

    auto err = ReadU64(bits);

    if (!err.Ok()) {
      return err;
    }

    static_assert(sizeof(bits) == sizeof(out));
    std::memcpy(&out, &bits, sizeof(out));

    return Success();
  }

  Error BinaryReader::ReadBool(bool &out) {
    std::uint8_t booleanByte;

    if (auto err = ReadU8(booleanByte);
            !err.Ok()) {

      return err;
    }

    if (booleanByte > 1) {
      return Fail(ErrorCode::InvalidData,
                  "Invalid bool value");
    }

    out = booleanByte == 1;
    return Success();
  }

  Error BinaryReader::ReadString(std::string& out) {
    std::uint32_t size = 0;

    auto err = ReadU32(size);

    if (!err.Ok()) {
      return err;
    }

    if (size > limits_.maxStringBytes) {
      return Fail(
        ErrorCode::SizeLimitExceeded,
        "String exceeded size limit."
      );
    }

    out.resize(size);

    if (size == 0) {
      return Success();
    }

    return ReadBytes(
      std::span<std::uint8_t>(
        reinterpret_cast<std::uint8_t*>(out.data()),
        out.size()
      )
    );
  }

  Error BinaryReader::ReadI32(std::int32_t& out) {
    std::uint8_t bytes[4] {};
    auto err = ReadBytes(bytes);

    if (!err.Ok()) {
      return err;
    }

    const std::uint32_t raw =
      static_cast<std::uint32_t>(bytes[0]) |
      (static_cast<std::uint32_t>(bytes[1]) << 8) |
      (static_cast<std::uint32_t>(bytes[2]) << 16) |
      (static_cast<std::uint32_t>(bytes[3]) << 24);

    out = std::bit_cast<std::int32_t>(raw);

    return Success();
  }
}
