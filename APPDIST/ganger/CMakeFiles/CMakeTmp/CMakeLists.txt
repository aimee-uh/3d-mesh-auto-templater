cmake_minimum_required(VERSION 3.3.2.0)
project(CMAKE_TRY_COMPILE C)
set(CMAKE_VERBOSE_MAKEFILE 1)
set(CMAKE_C_FLAGS "")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMPILE_DEFINITIONS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${EXE_LINKER_FLAGS}")
include_directories(${INCLUDE_DIRECTORIES})
set(CMAKE_SUPPRESS_REGENERATION 1)
link_directories(${LINK_DIRECTORIES})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "/homes/grail/curless/src/deformation/CMakeFiles/CMakeTmp")
add_executable(cmTC_00355 "/usr/share/cmake/Modules/CMakeCCompilerABI.c")
target_link_libraries(cmTC_00355 ${LINK_LIBRARIES})
