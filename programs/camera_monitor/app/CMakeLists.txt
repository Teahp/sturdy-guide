add_executable(camera_homework_app main.cpp)
target_link_libraries(camera_homework_app PRIVATE camera_homework_core)
camera_homework_enable_warnings(camera_homework_app)

# 保留原始程序的命令行名字和输出位置，原来怎么运行现在还怎么运行。
set_target_properties(camera_homework_app PROPERTIES
  OUTPUT_NAME sturdy-guide-camera
  RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
)
