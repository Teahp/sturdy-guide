# 摄像头作业：核心逻辑编译为库 target，应用与测试分别链接该库。
find_package(
  OpenCV 4
  REQUIRED
  COMPONENTS core highgui imgcodecs imgproc videoio
)
find_package(Threads REQUIRED)

# 起始版本保留在 starter/ 用于对照，默认不参与构建：
# 最终 target sturdy-guide-camera 不得编译 starter/main.cpp。
option(STURDY_GUIDE_BUILD_CAMERA_STARTER
       "Build the single-threaded starter for comparison" OFF)
if(STURDY_GUIDE_BUILD_CAMERA_STARTER)
  add_subdirectory(starter)
endif()

add_subdirectory(src)
add_subdirectory(app)

# include(CTest) 定义 BUILD_TESTING（默认 ON）并为本目录及以下启用测试，
# 使 tests/ 里的 add_test() 真正注册到 CTest。
include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
