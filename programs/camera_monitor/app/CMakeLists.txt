add_executable(sturdy-guide-camera
  main.cpp
  preview_application.cpp
)

target_include_directories(sturdy-guide-camera PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(sturdy-guide-camera PRIVATE cxx_std_17)
target_link_libraries(sturdy-guide-camera
  PRIVATE
    camera::camera_core
    ${OpenCV_LIBS}
)
