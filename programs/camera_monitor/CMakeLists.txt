# 摄像头作业：核心库、应用与测试分别在子目录中声明自己的 target。
# starter/ 保留在仓库中用于对照，但不再参与最终构建。
find_package(
  OpenCV 4
  REQUIRED
  COMPONENTS core highgui imgcodecs imgproc videoio
)
find_package(Threads REQUIRED)

add_subdirectory(src)
add_subdirectory(app)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
