# 摄像头回家作业：把单线程程序重构为「FrameSource / CameraSession / PreviewApplication」三层组件。
#
# 与其它 lesson programs 不同，这里只在 STURDY_GUIDE_BUILD_CAMERA_HOMEWORK=ON 时才会进入，
# 因此默认构建与 CI（不安装 OpenCV）都不受影响。

find_package(OpenCV 4 REQUIRED COMPONENTS core highgui imgcodecs imgproc videoio)
find_package(Threads REQUIRED)

set(CAMERA_HOMEWORK_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

# 与仓库根目录的 STURDY_GUIDE_ENABLE_WARNINGS 开关保持一致，避免这里自成一套规则。
function(camera_homework_enable_warnings target)
  if(STURDY_GUIDE_ENABLE_WARNINGS)
    if(MSVC)
      target_compile_options(${target} PRIVATE /W4)
    else()
      target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
  endif()
endfunction()

# 核心逻辑单独成库：应用和测试链接同一个 target，测试因此不需要复制实现。
add_subdirectory(src)
add_subdirectory(app)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
