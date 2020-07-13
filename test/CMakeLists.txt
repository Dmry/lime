find_package(Boost COMPONENTS log log_setup unit_test_framework REQUIRED)

set(lime_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src/)

file(GLOB_RECURSE src ${lime_SOURCE_DIR}/*.cpp )
list(REMOVE_ITEM src ${lime_SOURCE_DIR}/lime.cpp)

include_directories(${lime_SOURCE_DIR}/inc)
include_directories(${Boost_INCLUDE_DIR})

add_executable(lime_test ${src} ${cr_src} constraint_release.cpp)

target_link_libraries(lime_test -lm tbb ${Boost_LIBRARIES} Boost::boost GSL::gsl stdc++ Boost::unit_test_framework)

find_package(Threads REQUIRED)
if(THREADS_HAVE_PTHREAD_ARG)
  target_compile_options(lime_test PUBLIC "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
  target_link_libraries(lime_test "${CMAKE_THREAD_LIBS_INIT}")
endif()

enable_testing()

add_test(NAME boost_test COMMAND $<TARGET_FILE:lime_test>)