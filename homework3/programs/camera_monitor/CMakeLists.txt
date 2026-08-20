# 摄像头作业：把单文件起始程序重构为库、应用和测试。
# 起始版本保留在 starter/ 用于对照，不再参与最终 target 的编译。
find_package(
  OpenCV 4
  REQUIRED
  COMPONENTS core highgui imgcodecs imgproc videoio
)
find_package(Threads REQUIRED)

# 设备抽象、采集会话和主线程预览编译为库，CLI 与测试只依赖公开接口。
add_library(
  sturdy_guide_camera_core
  src/open_cv_camera.cpp
  src/camera_session.cpp
  src/preview_application.cpp
)
add_library(SturdyGuide::camera_core ALIAS sturdy_guide_camera_core)

target_compile_features(sturdy_guide_camera_core PUBLIC cxx_std_17)
target_include_directories(
  sturdy_guide_camera_core
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 依赖附着在真正需要的 target 上，并区分「头文件暴露(PUBLIC)」与「实现使用(PRIVATE)」：
# - opencv_core / opencv_videoio：FrameSource 与 OpenCvCamera 的头文件暴露了
#   cv::Mat 与 cv::VideoCapture，必须 PUBLIC 传递给下游；
# - opencv_highgui / imgcodecs / imgproc：只在 preview_application.cpp 内部使用，
#   保持 PRIVATE 不外泄。
# - Threads::Threads：camera_session.hpp 使用 std::thread，必须 PUBLIC。
target_link_libraries(
  sturdy_guide_camera_core
  PUBLIC
    opencv_core
    opencv_videoio
    Threads::Threads
  PRIVATE
    opencv_highgui
    opencv_imgcodecs
    opencv_imgproc
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

add_subdirectory(app)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
