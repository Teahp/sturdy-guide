cmake_minimum_required(VERSION 3.10)
project(CameraMonitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找依赖
find_package(OpenCV REQUIRED)
find_package(GTest)   # 可选，如果找不到也不会报错

# 全局包含目录（确保所有源文件都能找到 include/ 下的头文件）
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# ---------- 核心库 ----------
add_library(camera_core STATIC
    src/opencv_camera.cpp
    src/camera_session.cpp
)
target_link_libraries(camera_core ${OpenCV_LIBS})

# ---------- 主程序 ----------
add_executable(sturdy-guide-camera
    src/preview_application.cpp
    app/main.cpp
)
target_link_libraries(sturdy-guide-camera
    camera_core
    ${OpenCV_LIBS}
)

# ---------- 测试（可选） ----------
if(GTest_FOUND)
    add_executable(camera_test
        tests/fake_frame_source_test.cpp
        tests/camera_session_test.cpp
        src/fake_frame_source.cpp
    )
    target_link_libraries(camera_test
        camera_core
        ${OpenCV_LIBS}
        GTest::gtest
        GTest::gtest_main
    )
    add_test(NAME camera_test COMMAND camera_test)
endif()