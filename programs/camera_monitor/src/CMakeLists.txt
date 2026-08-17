# 核心逻辑库：设备抽象、采集会话与预览应用。
add_library(camera_core
  camera_session.cpp
  fake_frame_source.cpp
  opencv_camera.cpp
  preview_application.cpp
)
add_library(camera::core ALIAS camera_core)

target_compile_features(camera_core PUBLIC cxx_std_17)
target_include_directories(
  camera_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${OpenCV_INCLUDE_DIRS}
)

# 头文件暴露 cv::Mat 与 std::thread，因此 OpenCV 与 Threads 作为
# PUBLIC 依赖传递：应用和测试只需要链接 camera_core。
target_link_libraries(camera_core PUBLIC Threads::Threads ${OpenCV_LIBS})

if(STURDY_GUIDE_ENABLE_WARNINGS)
  if(MSVC)
    target_compile_options(camera_core PRIVATE /W4)
  else()
    target_compile_options(camera_core PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endif()
