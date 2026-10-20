add_library(camera_homework_core
  camera_session.cpp
  fake_frame_source.cpp
  opencv_camera.cpp
  preview_application.cpp
)

target_compile_features(camera_homework_core PUBLIC cxx_std_17)

# 自己的头文件用普通包含路径，OpenCV 的用 SYSTEM：这样 -Wall -Wextra 不会去报第三方头文件。
target_include_directories(camera_homework_core
  PUBLIC "${CAMERA_HOMEWORK_INCLUDE_DIR}"
)
target_include_directories(camera_homework_core
  SYSTEM PUBLIC ${OpenCV_INCLUDE_DIRS}
)

# PUBLIC：链接本库的可执行文件自动获得 OpenCV 和 Threads，
# 不需要再猜 OpenCV 的 .so 路径。
target_link_libraries(camera_homework_core
  PUBLIC ${OpenCV_LIBS} Threads::Threads
)

camera_homework_enable_warnings(camera_homework_core)
