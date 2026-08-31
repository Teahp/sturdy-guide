add_executable(camera_session_test test_camera_session.cpp)

target_link_libraries(camera_session_test
  PRIVATE camera_lib
)

add_test(NAME CameraSessionTests COMMAND camera_session_test)
