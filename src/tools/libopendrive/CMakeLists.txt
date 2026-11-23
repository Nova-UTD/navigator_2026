# Builds upstream libOpenDRIVE (in libOpenDRIVE/, never edited by hand) as a
# colcon package. Other packages use it with:
#   find_package(OpenDrive REQUIRED)
#   target_link_libraries(<target> OpenDrive::OpenDrive)
cmake_minimum_required(VERSION 3.14)
project(libopendrive)

find_package(ament_cmake REQUIRED)

# The workspace is built without a build type, which means no compiler
# optimisation. Default this package to Release (-O3) unless one is given.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Use the system pugixml (libpugixml-dev). Finding it here, before upstream's
# CMake runs, stops upstream from downloading its own copy during the build.
find_package(pugixml REQUIRED CONFIG)

add_subdirectory(libOpenDRIVE)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(test_libopendrive test/test_libopendrive.cpp)
  target_link_libraries(test_libopendrive OpenDrive::OpenDrive)
  target_compile_definitions(test_libopendrive PRIVATE
    FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test/fixtures")
endif()

ament_package()
