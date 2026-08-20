add_executable(sturdy_guide_camera_app main.cpp)
target_link_libraries(
  sturdy_guide_camera_app
  PRIVATE SturdyGuide::camera_core
)

# 保持起始版本的最终可执行文件名与输出位置。
set_target_properties(
  sturdy_guide_camera_app
  PROPERTIES
    OUTPUT_NAME sturdy-guide-camera
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
)
