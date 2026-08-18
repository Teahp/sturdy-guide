set(sturdy_guide_camera_sources
  camera_session.cpp
  fake_frame_source.cpp
  opencv_camera.cpp
  preview_application.cpp
)

add_library(sturdy_guide_camera_core ${sturdy_guide_camera_sources})
add_library(SturdyGuide::camera_core ALIAS sturdy_guide_camera_core)

target_compile_features(sturdy_guide_camera_core PUBLIC cxx_std_17)
target_include_directories(
  sturdy_guide_camera_core
  PUBLIC
    ${PROJECT_SOURCE_DIR}/programs/camera_monitor/include
    ${OpenCV_INCLUDE_DIRS}
)
target_link_libraries(
  sturdy_guide_camera_core
  PUBLIC
    Threads::Threads
    ${OpenCV_LIBS}
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
