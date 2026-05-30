//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_PATHSANITIZER_H
#define LIBSXP_PATHSANITIZER_H
#include <string>

namespace sxp {
  class PathSanitizer {
  public:
    static std::string MakeSafeDisplayName(const std::string& path);
    static bool IsSafeRelativePath(const std::string& path);
  };
}

#endif //LIBSXP_PATHSANITIZER_H
