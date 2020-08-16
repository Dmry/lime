message("-- Trying main targets")

add_subdirectory(constraint_release)

### DEPENDENCIES ###

# GSL
find_package(GSL 2.4 REQUIRED)
set(LIBS GSL::gsl)

# Python
find_package(pybind11)
if(pybind11_FOUND)
  message("-- Found PyBind11, shared library target is available")
  find_package(PythonLibs 3 REQUIRED)
  find_package(Python 3 COMPONENTS Interpreter Development NumPy REQUIRED)
  include_directories(${PYTHON_INCLUDE_DIRS})
  include_directories(${pybind11_INCLUDE_DIRS})
else()
  message("-- PyBind11 not found, shared library target unavailable")
endif()

# Boost
find_package(Boost 1.66.0 COMPONENTS log log_setup REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
set(LIBS ${LIBS} ${Boost_LIBRARIES} Boost::boost)
add_definitions(-DBOOST_LOG_DYN_LINK)

set(LIBS ${LIBS} stdc++)

# Intel TBB
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/../inc)
find_package(TBB 2018 REQUIRED)
if(TBB_FOUND)
  message("-- Multi-threading available and enabled")
  add_definitions(-DRUN_PARALLEL)
  set(LIBS ${LIBS} tbb)
else()
  message("-- Multi-threading NOT available")
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
list(REMOVE_ITEM src ${CMAKE_CURRENT_SOURCE_DIR}/lime.cpp)

# python
if(pybind11_FOUND)
  pybind11_add_module(lime_python SHARED ${src})
  target_link_libraries(lime_python PRIVATE ${LIBS} "m")
  install(TARGETS lime_python DESTINATION /usr/lib/python3.8)
  install(FILES ../gui/gui_glade.py DESTINATION bin RENAME lime-gui PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
  install(FILES ../gui/lime.glade DESTINATION share/lime/ PERMISSIONS WORLD_READ OWNER_WRITE OWNER_READ)
  install(FILES ../gui/lime.png DESTINATION share/lime/ PERMISSIONS WORLD_READ OWNER_WRITE OWNER_READ)
  install(FILES ../gui/lime.desktop DESTINATION share/applications/ PERMISSIONS WORLD_READ OWNER_READ GROUP_READ)
endif()

list(REMOVE_ITEM src ${CMAKE_CURRENT_SOURCE_DIR}/python.cpp)

add_library(lime_static STATIC ${src})
target_link_libraries(lime_static ${LIBS} "m")

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

install(TARGETS lime DESTINATION bin)

message("-- Main targets done")