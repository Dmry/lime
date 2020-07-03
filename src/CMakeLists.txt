include_directories(${ics_SOURCE_DIR}/inc)
include_directories( ${Boost_INCLUDE_DIR} )
include_directories( ${Eigen3_INCLUDE_DIR} )
link_directories(${ics_SOURCE_DIR}/src)

file(GLOB src "*.cpp" )

add_executable(ics ${src})

find_package(Threads REQUIRED)
if(THREADS_HAVE_PTHREAD_ARG)
  target_compile_options(ics PUBLIC "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
  target_link_libraries(ics "${CMAKE_THREAD_LIBS_INIT}")
endif()

target_link_libraries(ics -lm tbb ${Boost_LIBRARIES} Eigen3::Eigen GSL::gsl stdc++)