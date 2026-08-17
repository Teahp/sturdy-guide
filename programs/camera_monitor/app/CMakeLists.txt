# 应用层：参数解析、依赖组装和主窗口循环。
add_executable(sturdy_guide_camera main.cpp)
target_compile_features(sturdy_guide_camera PRIVATE cxx_std_17)
target_link_libraries(sturdy_guide_camera PRIVATE camera_core)

if(STURDY_GUIDE_ENABLE_WARNINGS)
  if(MSVC)
    target_compile_options(sturdy_guide_camera PRIVATE /W4)
  else()
    target_compile_options(
      sturdy_guide_camera
      PRIVATE -Wall -Wextra -Wpedantic
    )
  endif()
endif()

set_target_properties(
  sturdy_guide_camera
  PROPERTIES
    OUTPUT_NAME sturdy-guide-camera
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
)
