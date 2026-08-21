# Refactored camera_monitor: library, application, and tests.
# The original single-threaded starter is kept in starter/ for reference.

find_package(OpenCV 4 REQUIRED COMPONENTS core highgui imgcodecs imgproc videoio)
find_package(Threads REQUIRED)

set(sturdy_guide_camera_sources
  src/camera_session.cpp
  src/fake_frame_source.cpp
  src/opencv_camera.cpp
  src/preview_application.cpp
)

add_library(sturdy_guide_camera_core ${sturdy_guide_camera_sources})
add_library(SturdyGuide::camera_core ALIAS sturdy_guide_camera_core)

target_compile_features(sturdy_guide_camera_core PUBLIC cxx_std_17)
target_include_directories(
  sturdy_guide_camera_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
  sturdy_guide_camera_core
  PUBLIC ${OpenCV_LIBS} Threads::Threads
)

if(STURDY_GUIDE_ENABLE_WARNINGS)
  if(MSVC)
    target_compile_options(sturdy_guide_camera_core PRIVATE /W4)
  else()
    target_compile_options(
      sturdy_guide_camera_core
      PRIVATE -Wall -Wextra -Wpedantic
    )
  endif()
endif()

# The starter is kept for reference; its output is renamed to avoid conflict.
add_subdirectory(starter)
add_subdirectory(app)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
