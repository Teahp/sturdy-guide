add_library(camera_lib STATIC
  opencv_camera.cpp
  fake_frame_source.cpp
  camera_session.cpp
)

target_include_directories(camera_lib
  PUBLIC ${PROJECT_SOURCE_DIR}/include
  PRIVATE ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(camera_lib
  PUBLIC ${OpenCV_LIBS}
  PRIVATE Threads::Threads
)

target_compile_features(camera_lib PUBLIC cxx_std_17)
