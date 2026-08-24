# 摄像头作业：把起始单文件程序重构为库、应用与硬件无关测试。
find_package(
  OpenCV 4
  REQUIRED
  COMPONENTS core highgui imgcodecs imgproc videoio
   PATHS
    /usr/lib/x86_64-linux-gnu/cmake/opencv4
    /usr/local/lib/cmake/opencv4
  NO_DEFAULT_PATH
)
find_package(Threads REQUIRED)

add_subdirectory(src)
add_subdirectory(app)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
