# 核心逻辑编译为静态库；应用与测试分别链接该库。
add_library(sturdy_guide_camera_core STATIC
  opencv_camera.cpp
  fake_frame_source.cpp
  camera_session.cpp
  preview_app.cpp
)

target_include_directories(
  sturdy_guide_camera_core
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(
  sturdy_guide_camera_core
  PUBLIC ${OpenCV_LIBS} Threads::Threads
)

target_compile_features(sturdy_guide_camera_core PUBLIC cxx_std_17)

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
