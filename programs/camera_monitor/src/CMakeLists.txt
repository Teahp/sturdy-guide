# 核心逻辑编译为库：采集会话 + 真设备实现。
set(sturdy_guide_camera_sources
  opencv_camera.cpp
  camera_session.cpp
)

add_library(sturdy_guide_camera_core ${sturdy_guide_camera_sources})
add_library(SturdyGuide::camera_core ALIAS sturdy_guide_camera_core)

target_compile_features(sturdy_guide_camera_core PUBLIC cxx_std_17)
target_include_directories(
  sturdy_guide_camera_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${OpenCV_INCLUDE_DIRS}
)
# OpenCV 与线程依赖必须传递给使用该库的应用和测试。
target_link_libraries(
  sturdy_guide_camera_core
  PUBLIC
    ${OpenCV_LIBS}
    Threads::Threads
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
