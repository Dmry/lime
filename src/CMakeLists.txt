add_subdirectory(constraint_release)

### DEPENDENCIES ###

# GSL
find_package(GSL REQUIRED)
set(LIBS GSL::gsl)

# Eigen
find_package(Eigen3 REQUIRED)
set(LIBS ${LIBS} ${Eigen_LIBRARIES})

# Ceres
find_package(Ceres REQUIRED)
set(LIBS ${LIBS} ${Ceres_LIBRARIES})
include_directories(${CERES_INCLUDE_DIRS})

# Boost
find_package(Boost COMPONENTS log log_setup REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
set(LIBS ${LIBS} ${Boost_LIBRARIES} Boost::boost)
add_definitions(-DBOOST_LOG_DYN_LINK)

set(LIBS ${LIBS} stdc++)

# Intel TBB
find_package(TBB)
if(TBB_FOUND)
  add_definitions(-DRUN_PARALLEL)
  set(LIBS ${LIBS} tbb)
endif()

# Threads
find_package(Threads REQUIRED)

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
target_link_libraries(lime lime_static)
if (CUDA_FOUND AND CUDA)
  target_link_libraries(lime lime_cuda_static)
endif()

if(THREADS_HAVE_PTHREAD_ARG)
  target_compile_options(lime PUBLIC "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
  target_link_libraries(lime "${CMAKE_THREAD_LIBS_INIT}")
endif()