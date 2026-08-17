cmake_minimum_required(VERSION 3.10)
project(camera_monitor VERSION 0.1.0 LANGUAGES CXX)

find_package(OpenCV REQUIRED COMPONENTS core videoio highgui)
find_package(Threads REQUIRED)

add_library(camera_lib
    src/opencv_camera.cpp
    src/session.cpp
    src/fake_frame_source.cpp
)

target_include_directories(camera_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(camera_lib
    ${OpenCV_LIBS}
    Threads::Threads
)

add_executable(sturdy_guide_camera app/main.cpp)
target_link_libraries(sturdy_guide_camera
    camera_lib
    ${OpenCV_LIBS}
    Threads::Threads
)

# 添加测试子目录（仅在开启测试时）
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
