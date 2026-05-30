//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_ERROR_H
#define LIBSXP_ERROR_H

#include <string>

#include "ErrorCode.h"

namespace sxp {
  struct Error {
    ErrorCode code = ErrorCode::None;
    std::string message;

    bool Ok() const;
  };

  Error Success();
  Error Fail(ErrorCode code, std::string message);
}

#endif // LIBSXP_ERROR_H