# 起始版本只有一个单线程可执行程序。作业要求学生把它重构为库、应用和测试。
find_package(
  OpenCV 4
  REQUIRED
  COMPONENTS core highgui imgcodecs imgproc videoio
)
find_package(Threads REQUIRED)

#add_subdirectory(starter)
add_subdirectory(src)
add_subdirectory(app)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
