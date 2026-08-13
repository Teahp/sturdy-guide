# 课程程序各自维护内部 target；本层只负责编排。
add_subdirectory(midi_player)

# 摄像头起始程序依赖 OpenCV，默认关闭以免影响原课程和 SDK 构建。
if(STURDY_GUIDE_BUILD_CAMERA_HOMEWORK)
  add_subdirectory(camera_monitor)
endif()
