# 摄像头作业：核心逻辑编译为库 target，应用和测试分别链接。
find_package(OpenCV 4 REQUIRED COMPONENTS core highgui imgcodecs imgproc videoio)
find_package(Threads REQUIRED)

# 中文 Windows 上强制 MSVC 按 UTF-8 解析源文件，避免 GBK 误读注释。
if(MSVC)
  add_compile_options(/utf-8)
endif()

add_library(camera_core
  src/opencv_camera.cpp
  src/fake_frame_source.cpp
  src/camera_session.cpp
)
add_library(camera::camera_core ALIAS camera_core)

target_include_directories(camera_core PUBLIC include)
target_compile_features(camera_core PUBLIC cxx_std_17)
target_link_libraries(camera_core
  PUBLIC
    ${OpenCV_LIBS}
    Threads::Threads
)

add_subdirectory(app)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
