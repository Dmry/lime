add_subdirectory(constraint_release)

### DEPENDENCIES ###

# GSL
find_package(GSL REQUIRED)
set(LIBS GSL::gsl)

# Boost
find_package(Boost COMPONENTS log log_setup REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
set(LIBS ${LIBS} ${Boost_LIBRARIES} Boost::headers)
add_definitions(-DBOOST_LOG_DYN_LINK)

set(LIBS ${LIBS} stdc++)

# Intel TBB
find_package(TBB)
if(TBB_FOUND)
  add_definitions(-DRUN_PARALLEL)
  set(LIBS ${LIBS} tbb)
endif()

# CUDA
find_package(CUDA)
if(CUDA_FOUND)
  add_definitions(-DCUDA)
endif()

### SHARED OBJECT ###

# cpp
file(GLOB_RECURSE src ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp )
list(REMOVE_ITEM src ${lime_SOURCE_DIR}/lime.cpp)

add_library(lime_static STATIC ${src})
target_link_libraries(lime_static ${LIBS})

### EXECUTABLE ###
add_executable(lime lime.cpp)
target_link_libraries(lime lime_static lime_cuda_static)