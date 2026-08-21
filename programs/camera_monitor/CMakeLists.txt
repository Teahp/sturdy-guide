# 起始版本只有一个单线程可执行程序。作业要求学生把它重构为库、应用和测试。
find_package(
  OpenCV 4
  REQUIRED
  COMPONENTS core highgui imgcodecs imgproc videoio
)

# 核心库
add_library(camera_core STATIC
  ${CMAKE_SOURCE_DIR}/src/FakeFrameSource.cpp
  ${CMAKE_SOURCE_DIR}/src/CameraSession.cpp
  ${CMAKE_SOURCE_DIR}/src/OpenCVCamera.cpp
)
target_include_directories(camera_core
  PUBLIC ${CMAKE_SOURCE_DIR}/include
  PRIVATE ${OpenCV_INCLUDE_DIRS}
)
target_link_libraries(camera_core
  PRIVATE ${OpenCV_LIBS}
)
target_compile_features(camera_core PRIVATE cxx_std_17)

# 测试程序：验证 FakeFrameSource
add_executable(test_fake ${CMAKE_SOURCE_DIR}/src/test_fake.cpp)
target_link_libraries(test_fake camera_core ${OpenCV_LIBS})
target_include_directories(test_fake PRIVATE ${CMAKE_SOURCE_DIR}/include ${OpenCV_INCLUDE_DIRS})
target_compile_features(test_fake PRIVATE cxx_std_17)

# 重构后的应用
add_executable(camera_app ${CMAKE_SOURCE_DIR}/src/camera_monitor_main.cpp)
target_link_libraries(camera_app camera_core ${OpenCV_LIBS})
target_include_directories(camera_app PRIVATE ${CMAKE_SOURCE_DIR}/include ${OpenCV_INCLUDE_DIRS})
target_compile_features(camera_app PRIVATE cxx_std_17)
# 起始程序（后续会被重构后的应用替代）
add_subdirectory(starter)
