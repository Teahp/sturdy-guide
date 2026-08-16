# 硬件无关测试：链接核心库，注册到 CTest。
add_executable(camera_session_test test_camera_session.cpp)

target_link_libraries(camera_session_test PRIVATE sturdy_guide_camera_core)

target_compile_features(camera_session_test PRIVATE cxx_std_17)

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
