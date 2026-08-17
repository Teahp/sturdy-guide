# 硬件无关测试：通过 FakeFrameSource 驱动 CameraSession，不打开真实设备。
add_executable(camera_session_test camera_session_test.cpp)
target_compile_features(camera_session_test PRIVATE cxx_std_17)
target_link_libraries(camera_session_test PRIVATE camera_core)
add_test(NAME camera_session_test COMMAND camera_session_test)

if(STURDY_GUIDE_ENABLE_WARNINGS)
  if(MSVC)
    target_compile_options(camera_session_test PRIVATE /W4)
  else()
    target_compile_options(
      camera_session_test
      PRIVATE -Wall -Wextra -Wpedantic
    )
  endif()
endif()
