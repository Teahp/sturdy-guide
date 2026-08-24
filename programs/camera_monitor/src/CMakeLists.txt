  add_library(sturdy_guide_camera_core camera_session.cpp opencv_camera.cpp)
  add_library(SturdyGuide::camera_core ALIAS sturdy_guide_camera_core)
  target_compile_features(sturdy_guide_camera_core PUBLIC cxx_std_17)
  target_include_directories(sturdy_guide_camera_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include ${OpenCV_INCLUDE_DIRS})
  target_link_libraries(sturdy_guide_camera_core PUBLIC ${OpenCV_LIBS} Threads::Threads)
 