# CUDA
find_package(CUDA)
if(CUDA_FOUND)
  add_definitions(-DCUDA)
endif()

file(GLOB_RECURSE cu_src ${CMAKE_CURRENT_SOURCE_DIR}/*.cu)
enable_language(CUDA)

set(CMAKE_CUDA_STANDARD 14)
set(CMAKE_CUDA_STANDARD_REQUIRED TRUE)
set(CMAKE_CUDA_FLAGS "-arch=sm_61 --default-stream per-thread")
set(CMAKE_CUDA_FLAGS_RELEASE "-O3")
set(CMAKE_CUDA_FLAGS_DEBUG "-G -g")
if(NOT DEFINED ${CMAKE_CUDA_ARCHITECTURES})
    set(CMAKE_CUDA_ARCHITECTURES 61)
endif()

find_library(CUDART_LIBRARY cudart ${CMAKE_CUDA_IMPLICIT_LINK_DIRECTORIES})

add_library(lime_cuda_static STATIC ${cu_src})
target_include_directories(
    lime_cuda_static 
    PRIVATE 
    ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}
)
target_link_libraries(lime_cuda_static ${CUDART_LIBRARY})