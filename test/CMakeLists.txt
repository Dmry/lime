find_package(Catch2 REQUIRED)

file(GLOB src ${ics_SOURCE_DIR}/src/*.cpp)
file(GLOB test_src ${ics_SOURCE_DIR}/test/*.cpp)
list(REMOVE_ITEM src ${ics_SOURCE_DIR}/src/ics.cpp)

add_executable (ics_test ${src} ${test_src})

target_link_libraries(ics_test Catch2::Catch2)
target_link_libraries(ics_test GSL::gsl)

if(THREADS_HAVE_PTHREAD_ARG)
  target_compile_options(ics_test PUBLIC "-pthread")
endif()

target_link_libraries(ics_test tbb ${Boost_LIBRARIES})

add_definitions(-DTEST)

include(CTest)
include(Catch)

link_directories(${ics_SOURCE_DIR}/src)
include_directories(${ics_SOURCE_DIR}/inc)

catch_discover_tests(ics_test)