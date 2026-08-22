# 硬件无关测试：通过 FakeFrameSource 注入假设备，不打开真实摄像头。
add_executable(camera_session_test
  camera_session_test.cpp
  fake_frame_source.cpp
)
target_link_libraries(camera_session_test PRIVATE SturdyGuide::camera_core)
add_test(NAME camera_session_test COMMAND camera_session_test)
