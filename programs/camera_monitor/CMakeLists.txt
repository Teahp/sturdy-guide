# 摄像头作业：重构后的库、应用和测试
find_package(
  OpenCV 4
  REQUIRED
  COMPONENTS core highgui imgcodecs imgproc videoio
)
find_package(Threads REQUIRED)

# 编排子目录
add_subdirectory(src)
add_subdirectory(app)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
