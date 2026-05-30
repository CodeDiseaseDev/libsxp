//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_RESULT_H
#define LIBSXP_RESULT_H

#include "core/Error.h"

namespace sxp {
  template<typename T>
  struct Result {
    T value {};
    Error error {};

    bool Ok() const {
      return error.Ok();
    }
  };
}


#endif //LIBSXP_RESULT_H
