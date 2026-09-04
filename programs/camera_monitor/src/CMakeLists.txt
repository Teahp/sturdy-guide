add_library(camera_core fake_frame_source.cpp opencv_camera.cpp camera_session.cpp)
add_library(SturdyGuide::camera_core ALIAS camera_core)

target_compile_features(camera_core PUBLIC cxx_std_17)
target_include_directories(
    camera_core
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
target_link_libraries(camera_core PUBLIC ${OpenCV_LIBS} Threads::Threads)