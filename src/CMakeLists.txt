add_subdirectory(constraint_release)

include_directories(${lime_SOURCE_DIR}/inc)
include_directories(${Boost_INCLUDE_DIR})
include_directories(${Eigen3_INCLUDE_DIR})

set(lime_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src/)

file(GLOB_RECURSE src ${lime_SOURCE_DIR}/*.cpp )

add_executable(lime ${src})

find_package(Threads REQUIRED)
if(THREADS_HAVE_PTHREAD_ARG)
  target_compile_options(lime PUBLIC "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
  target_link_libraries(lime "${CMAKE_THREAD_LIBS_INIT}")
endif()

add_definitions(-DBOOST_LOG_DYN_LINK)

# TODO: make tbb dependency optional

target_link_libraries(lime -lm tbb ${Boost_LIBRARIES} Boost::boost GSL::gsl stdc++)