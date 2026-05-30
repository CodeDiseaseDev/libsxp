//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_ERRORCODE_H
#define LIBSXP_ERRORCODE_H

namespace sxp {
  enum class ErrorCode {
    None = 0,

    FileOpenFailed,
    FileReadFailed,
    FileWriteFailed,

    InvalidMagic,
    UnsupportedVersion,
    InvalidChunk,
    InvalidSize,
    SizeLimitExceeded,
    ChecksumFailed,
    InvalidData,

    MissingRequiredChunk,
    AssetNotFound
  };
}

#endif //LIBSXP_ERRORCODE_H
