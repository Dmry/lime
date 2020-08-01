cmake_minimum_required(VERSION 3.10)
project(lime VERSION 0.0.1 LANGUAGES CXX)

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

set(CPACK_PACKAGE_VERSION ${CMAKE_PROJECT_VERSION})
set(CPACK_GENERATOR "RPM")
set(CPACK_PACKAGE_NAME "Lime")
set(CPACK_PACKAGE_RELEASE 1)
set(CPACK_PACKAGE_CONTACT "Daniël Emmery")
set(CPACK_PACKAGE_VENDOR "CNRS")
set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pkg/rpm/lime.spec.in ${CMAKE_CURRENT_BINARY_DIR}/lime.spec @ONLY IMMEDIATE)
set(CPACK_RPM_USER_BINARY_SPECFILE ${CMAKE_CURRENT_BINARY_DIR}/lime.spec)
include(CPack)