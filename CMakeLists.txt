cmake_minimum_required(VERSION 3.10)
project(lime VERSION 0.1 LANGUAGES CXX)

### BUILD CONFIG ###

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED on)

set(CMAKE_CXX_FLAGS "-O3 -march=native -pipe -Wall -Wextra -Wfloat-conversion")
set(CMAKE_CXX_FLAGS_DEBUG "-g -march=native -pipe -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mtune=znver1 -pipe")

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -C ${CMAKE_CFG_INTDIR})

# Source dir
add_subdirectory(src)
set(lime_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src/)

#add_subdirectory(benchmark)
add_subdirectory(test)

# Local includes
add_subdirectory(inc)
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/inc)
include_directories(${CMAKE_MODULE_PATH})