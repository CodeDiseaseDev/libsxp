//
// Created by code on 5/30/26.
//

#ifndef LIBSXP_PROJECTGENERATOR_H
#define LIBSXP_PROJECTGENERATOR_H
#include <cstdint>
#include <string>
#include <vector>

#include "ProjectGeneratorType.h"

namespace sxp {
  struct ProjectGenerator {
    std::uint64_t nodeId = 0;
    std::string name;

    ProjectGeneratorType type = ProjectGeneratorType::Unknown;
    std::uint32_t version = 1;

    std::vector<std::uint8_t> payload;
  };
}

#endif //LIBSXP_PROJECTGENERATOR_H
