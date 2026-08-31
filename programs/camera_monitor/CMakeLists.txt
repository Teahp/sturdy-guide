cmake_minimum_required(VERSION 3.16)
project(sturdy_guide_camera LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(
  OpenCV 4
  REQUIRED
  COMPONENTS core highgui imgcodecs imgproc videoio
)
find_package(Threads REQUIRED)

option(STURDY_GUIDE_BUILD_CAMERA_HOMEWORK "Build refactored camera homework" OFF)

if(STURDY_GUIDE_BUILD_CAMERA_HOMEWORK)
  add_subdirectory(src)
  add_subdirectory(app)
  enable_testing()
  add_subdirectory(tests)
else()
  add_subdirectory(starter)
endif()

if(STURDY_GUIDE_ENABLE_WARNINGS)
  if(MSVC)
    add_compile_options(/W4)
  else()
    add_compile_options(-Wall -Wextra -Wpedantic)
  endif()
endif()
