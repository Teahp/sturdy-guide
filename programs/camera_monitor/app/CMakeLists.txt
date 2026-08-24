add_executable(sturdy_guide_camera main.cpp preview_application.cpp)
target_link_libraries(sturdy_guide_camera PRIVATE SturdyGuide::camera_core ${OpenCV_LIBS})
set_target_properties(sturdy_guide_camera PROPERTIES OUTPUT_NAME sturdy-guide-camera)
set_target_properties(sturdy_guide_camera PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
 