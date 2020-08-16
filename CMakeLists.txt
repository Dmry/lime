cmake_minimum_required(VERSION 3.10)
project(lime VERSION 1.0.0 LANGUAGES CXX)

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

### PACKAGING ###

set(CPACK_PACKAGE_VERSION ${CMAKE_PROJECT_VERSION})
set(CPACK_GENERATOR "RPM")
set(CPACK_PACKAGE_NAME "Lime")
set(CPACK_PACKAGE_RELEASE 1)
set(CPACK_PACKAGE_CONTACT "Daniël Emmery")
set(CPACK_PACKAGE_VENDOR "CNRS")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Tool for fitting simulations data with Likhtman & McLeish' model.")

# RPM
set(CPACK_RPM_PACKAGE_LICENSE "GPLv3")
set(CPACK_RPM_PACKAGE_REQUIRES "tbb >= 2018, gsl => 2.4, boost >= 1.66, python, python3-matplotlib, python3-matplotlib-gtk3, python3-gobject, python-numpy")
#RPM:
#BuildRequires:  gcc >= 9, cmake >= 3.0, tbb-devel >= 2018, gsl-devel >= 2.4, boost-devel >= 1.66, python-devel, pybind11-devel

include(CPack)