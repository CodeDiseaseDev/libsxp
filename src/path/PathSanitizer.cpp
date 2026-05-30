#include "sxp/path/PathSanitizer.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <sstream>
#include <vector>

namespace sxp {
  std::string PathSanitizer::MakeSafeDisplayName(
    const std::string& path
  ) {
    std::string normalized = path;

    std::replace(
      normalized.begin(),
      normalized.end(),
      '\\',
      '/'
    );

    const std::size_t slash =
      normalized.find_last_of('/');

    std::string name =
      slash == std::string::npos
        ? normalized
        : normalized.substr(slash + 1);

    if (name.empty()) {
      name = "asset";
    }

    for (char& c : name) {
      const unsigned char uc =
        static_cast<unsigned char>(c);

      const bool invalid =
        std::iscntrl(uc) ||
        c == '<' ||
        c == '>' ||
        c == ':' ||
        c == '"' ||
        c == '/' ||
        c == '\\' ||
        c == '|' ||
        c == '?' ||
        c == '*';

      if (invalid) {
        c = '_';
      }
    }

    if (name == "." || name == "..") {
      return "asset";
    }

    return name;
  }

  bool PathSanitizer::IsSafeRelativePath(
    const std::string& path
  ) {
    if (path.empty()) {
      return false;
    }

    if (path[0] == '/' || path[0] == '\\') {
      return false;
    }

    if (path.size() >= 2 &&
        std::isalpha(static_cast<unsigned char>(path[0])) &&
        path[1] == ':') {
      return false;
    }

    std::string normalized = path;

    std::replace(
      normalized.begin(),
      normalized.end(),
      '\\',
      '/'
    );

    std::stringstream stream(normalized);
    std::string part;

    while (std::getline(stream, part, '/')) {
      if (part.empty() || part == "." || part == "..") {
        return false;
      }
    }

    return true;
  }
}