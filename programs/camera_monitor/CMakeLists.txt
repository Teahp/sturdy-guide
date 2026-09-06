include(CTest)
find_package(Threads REQUIRED)

find_package(OpenCV 4 REQUIRED COMPONENTS
    core
    highgui
    imgcodecs
    imgproc
    videoio
)

add_library(sturdy_guide_camera_core
    src/opencv_camera.cpp
    src/camera_session.cpp
    src/fake_frame_source.cpp
    src/preview_application.cpp
)

target_include_directories(sturdy_guide_camera_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(sturdy_guide_camera_core
    PUBLIC
        Threads::Threads
        ${OpenCV_LIBS}
)

target_compile_features(
    sturdy_guide_camera_core
    PUBLIC cxx_std_17
)

add_subdirectory(starter)
add_subdirectory(app)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
