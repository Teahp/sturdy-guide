add_executable(camera_monitor_test camera_monitor_test.cpp)
target_link_libraries(
  camera_monitor_test
  PRIVATE SturdyGuide::camera_core
)
add_test(NAME camera_monitor_test COMMAND camera_monitor_test)
