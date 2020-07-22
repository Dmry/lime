find_package(Boost COMPONENTS unit_test_framework REQUIRED)
add_executable(lime_test test.cpp)

# Intel TBB
find_package(TBB)
if(TBB_FOUND)
  add_definitions(-DRUN_PARALLEL)
endif()

target_link_libraries(lime_test lime_static Boost::unit_test_framework)

enable_testing()

add_test(NAME boost_test COMMAND $<TARGET_FILE:lime_test>)