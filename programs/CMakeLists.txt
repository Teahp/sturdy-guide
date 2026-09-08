cmake_minimum_required(VERSION 3.15)
project(camera_monitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)
find_package(Threads REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(camera_core STATIC
        src/open_cv_camera.cpp
        src/camera_session.cpp
)

target_link_libraries(camera_core
        ${OpenCV_LIBS}
        Threads::Threads
)

add_executable(sturdy-guide-camera
        app/main.cpp
)

target_link_libraries(sturdy-guide-camera
        camera_core
        ${OpenCV_LIBS}
        Threads::Threads
)

enable_testing()