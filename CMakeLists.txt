cmake_minimum_required(VERSION 4.2)

project(libsxp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE LIBSXP_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
)

file(GLOB_RECURSE LIBSXP_HEADERS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)

add_library(sxp STATIC
        ${LIBSXP_SOURCES}
        ${LIBSXP_HEADERS}
        include/sxp/project/ProjectAsset.h
        include/sxp/project/generator_payload/OscillatorPayload.h
        include/sxp/project/generator_payload/SamplerPayload.h
        include/sxp/project/generator_payload/GeneratorPayloadCodec.h
        src/project/generator_payload/GeneratorPayloadCodec.cpp
        include/sxp/project/generator_payload/PayloadReaderContext.h
)

target_include_directories(sxp
        PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_features(sxp
        PUBLIC
        cxx_std_20
)

add_executable(sxp_test
        main_big_sxp_test.cpp
)

target_link_libraries(sxp_test
        PRIVATE
        sxp
)
