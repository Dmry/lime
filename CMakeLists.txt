cmake_minimum_required(VERSION 3.10)
project(ics VERSION 0.1 LANGUAGES CXX)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED on)

find_package(TBB)
find_package(Threads)
find_package(GSL REQUIRED)
find_package(Boost REQUIRED)
find_package(Eigen3 3.3 REQUIRED NO_MODULE)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(THREADS_FOUND AND TBB_FOUND)
  add_definitions(-DRUN_PARALLEL)
endif()

set(CMAKE_CXX_FLAGS "-g -O3 -march=native -pipe -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g -march=native -pipe -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -march=native -pipe -Wall -Wextra")

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_subdirectory(src)
add_subdirectory(benchmark)
add_subdirectory(inc)
add_subdirectory(test)

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -C ${CMAKE_CFG_INTDIR})
