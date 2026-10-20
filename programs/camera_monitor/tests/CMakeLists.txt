# 与仓库根目录 tests/CMakeLists.txt 的写法一致：
# 一个辅助函数负责「建可执行文件 + 链接核心库 + 注册到 CTest」这三件事。
function(add_camera_homework_test target source)
  add_executable(${target} ${source})
  target_link_libraries(${target} PRIVATE camera_homework_core)
  camera_homework_enable_warnings(${target})
  add_test(NAME ${target} COMMAND ${target})
endfunction()

# 全部是硬件无关测试：不会打开摄像头 0，也不需要图形窗口。
add_camera_homework_test(camera_session_test camera_session_test.cpp)
add_camera_homework_test(fake_frame_source_test fake_frame_source_test.cpp)
