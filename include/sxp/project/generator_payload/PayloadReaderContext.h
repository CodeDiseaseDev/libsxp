//
// Created by code on 6/1/26.
//

#ifndef LIBSXP_PAYLOADREADERCONTEXT_H
#define LIBSXP_PAYLOADREADERCONTEXT_H
#include <sstream>
#include <string>

#include <sxp/stream/BinaryReader.h>

namespace sxp {
  struct PayloadReaderContext {
    std::string data;
    std::istringstream stream;
    BinaryReader reader;

    explicit PayloadReaderContext(std::span<const std::uint8_t> bytes)
      : data(
          reinterpret_cast<const char*>(bytes.data()),
          bytes.size()
        ),
        stream(
          data,
          std::ios::in | std::ios::binary
        ),
        reader(stream) {
    }
  };
}

#endif //LIBSXP_PAYLOADREADERCONTEXT_H
