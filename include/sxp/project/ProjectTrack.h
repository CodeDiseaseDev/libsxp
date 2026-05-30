//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_PROJECTTRACK_H
#define LIBSXP_PROJECTTRACK_H
#include <cstdint>
#include <string>

namespace sxp {
  struct ProjectTrack {
    std::uint64_t id = 0;
    std::string name;

    std::vector<std::uint64_t> generatorNodeIds;
  };
}

#endif //LIBSXP_PROJECTTRACK_H
