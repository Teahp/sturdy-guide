# 课程程序各自维护内部 target；本层只负责编排。
# 顶层启用 CTest（不定义 BUILD_TESTING，避免改变各子目录既有行为），
# 使 camera_monitor 等子目录的 add_test() 能被 ctest --test-dir 发现。
enable_testing()

add_subdirectory(midi_player)

# 摄像头起始程序依赖 OpenCV，默认关闭以免影响原课程和 SDK 构建。
if(STURDY_GUIDE_BUILD_CAMERA_HOMEWORK)
  add_subdirectory(camera_monitor)
endif()
