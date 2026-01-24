cmake_minimum_required(VERSION 3.10)
include(ExternalProject)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(keikimix CXX C)

set(LIB_INCLUDE)
set(LIB_LINK)

# shogle
add_subdirectory("lib/shogle")
list(APPEND LIB_INCLUDE shogle)
list(APPEND LIB_LINK shogle)

# chimatools
ExternalProject_Add(chimatools
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/chimatools"
  BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/lib/chimatools"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND make -C <SOURCE_DIR> BUILD_DIR=<BINARY_DIR> lib
  BUILD_BYPRODUCTS <BINARY_DIR>/libchimatools.a
  INSTALL_COMMAND ""
)
list(APPEND LIB_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/lib/chimatools/include")
list(APPEND LIB_LINK "${CMAKE_CURRENT_BINARY_DIR}/lib/chimatools/libchimatools.a")

file(GLOB_RECURSE SOURCE_FILES "src/*.cpp")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -Wall -Wextra -Wpedantic -O0 -g2 -ggdb -Wno-psabi")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -Wall -Wextra -Wpedantic -O3 -Wno-psabi")

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC lib src ${LIB_INCLUDE})
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20)
target_link_libraries(${PROJECT_NAME} ${LIB_LINK})
