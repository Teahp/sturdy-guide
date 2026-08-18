cmake_minimum_required(VERSION 3.20)
project(camera_monitor_homework LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV REQUIRED)
find_package(Threads REQUIRED)

add_library(camera_monitor_core
    src/camera_session.cpp
    src/fake_frame_source.cpp
    src/opencv_camera.cpp
)

target_include_directories(camera_monitor_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(camera_monitor_core
    PUBLIC
        ${OpenCV_LIBS}
        Threads::Threads
)

add_executable(camera_monitor
    starter/main.cpp
)

target_link_libraries(camera_monitor
    PRIVATE
        camera_monitor_core
)

add_executable(camera_monitor_test
    tests/camera_session_test.cpp
)

target_link_libraries(camera_monitor_test
    PRIVATE
        camera_monitor_core
)

enable_testing()
add_test(NAME camera_monitor_test COMMAND camera_monitor_test)
