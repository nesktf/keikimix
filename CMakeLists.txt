cmake_minimum_required(VERSION 3.10)
include(ExternalProject)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(keikimix CXX C)

set(LIB_INCLUDE)
set(LIB_LINK)

# shogle
set(SHOGLE_ENABLE_IMGUI ON)
add_subdirectory("lib/shogle")
list(APPEND LIB_LINK shogle)

# chimatools
add_subdirectory("lib/chimatools")
list(APPEND LIB_LINK chimatools)

file(GLOB_RECURSE SOURCE_FILES "src/*.cpp")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -Wall -Wextra -Wpedantic -O0 -g2 -ggdb -Wno-psabi")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -Wall -Wextra -Wpedantic -O3 -Wno-psabi")

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC lib src ${LIB_INCLUDE})
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20)
target_link_libraries(${PROJECT_NAME} ${LIB_LINK})
