# 摄像头作业：核心逻辑编译为库，应用与测试分别链接该库。
find_package(
  OpenCV 4
  REQUIRED
  COMPONENTS core highgui imgcodecs imgproc videoio
)
find_package(Threads REQUIRED)

add_library(sturdy_guide_camera_core
  src/camera_session.cpp
  src/fake_frame_source.cpp
  src/opencv_camera.cpp
)
add_library(SturdyGuide::camera_core ALIAS sturdy_guide_camera_core)

target_compile_features(sturdy_guide_camera_core PUBLIC cxx_std_17)
target_include_directories(
  sturdy_guide_camera_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
)
target_link_libraries(
  sturdy_guide_camera_core
  PUBLIC Threads::Threads
  PUBLIC ${OpenCV_LIBS}
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

# starter/ 保留作对照，但不再参与构建；最终 target 只来自 app/。
add_subdirectory(app)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
