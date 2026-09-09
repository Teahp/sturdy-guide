# 硬件无关测试
add_executable(camera_session_tests
  test_camera_session.cpp
)

target_include_directories(camera_session_tests
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(camera_session_tests
  PRIVATE
    camera_core
)

target_compile_features(camera_session_tests PRIVATE cxx_std_17)

if(STURDY_GUIDE_ENABLE_WARNINGS)
  target_compile_options(camera_session_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME camera_session_tests COMMAND camera_session_tests)
