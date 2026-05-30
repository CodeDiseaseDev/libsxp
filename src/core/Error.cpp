#include "core/Error.h"

#include <utility>

namespace sxp {
  bool Error::Ok() const {
    return code == ErrorCode::None;
  }

  Error Success() {
    return {};
  }

  Error Fail(ErrorCode code, std::string message) {
    return Error {
      code,
      std::move(message)
    };
  }
}
