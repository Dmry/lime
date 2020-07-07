add_subdirectory(constraint_release)

include_directories(${lime_SOURCE_DIR}/inc)
include_directories( ${Boost_INCLUDE_DIR} )
include_directories( ${Eigen3_INCLUDE_DIR} )

link_directories(${lime_SOURCE_DIR}/src)

file(GLOB src "*.cpp" )
file(GLOB cr_src "constraint_release/*.cpp" )

add_executable(lime ${src} ${cr_src})

find_package(Threads REQUIRED)
if(THREADS_HAVE_PTHREAD_ARG)
  target_compile_options(ics PUBLIC "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
  target_link_libraries(lime "${CMAKE_THREAD_LIBS_INIT}")
endif()

target_link_libraries(lime -lm tbb ${Boost_LIBRARIES} Boost::boost GSL::gsl stdc++)