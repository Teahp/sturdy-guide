find_package(OpenCV 4 REQUIRED COMPONENTS core highgui imgcodecs imgproc videoio)
find_package(Threads REQUIRED)

add_library(camera_core STATIC
    src/fake_frame_source.cpp
    src/open_cv_camera.cpp
    src/camera_session.cpp
    src/preview_application.cpp
)

target_include_directories(camera_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        ${OpenCV_INCLUDE_DIRS}          # 关键：将 OpenCV 头文件路径公开
)

target_link_libraries(camera_core
    PUBLIC
        ${OpenCV_LIBS}                  # 使用变量，并设为 PUBLIC
        Threads::Threads
)

set_target_properties(camera_core PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

add_executable(sturdy_guide_camera app/main.cpp)
target_link_libraries(sturdy_guide_camera PRIVATE camera_core)

enable_testing()
add_executable(camera_tests tests/test_camera.cpp)
target_link_libraries(camera_tests PRIVATE camera_core)
add_test(NAME CameraTests COMMAND camera_tests)
