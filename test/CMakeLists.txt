find_package(Boost 1.66.0 COMPONENTS unit_test_framework REQUIRED)
add_executable(lime_test test.cpp)

# Intel TBB
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/../inc)
find_package(TBB 2018)
if(TBB_FOUND)
  add_definitions(-DRUN_PARALLEL)
endif()

target_link_libraries(lime_test lime_static Boost::unit_test_framework)

enable_testing()

add_test(NAME boost_test COMMAND $<TARGET_FILE:lime_test>)