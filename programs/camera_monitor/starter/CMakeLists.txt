# 这是供学生重构的单文件起始 target，不是目标工程结构。
add_executable(sturdy_guide_camera_starter main.cpp)
target_compile_features(sturdy_guide_camera_starter PRIVATE cxx_std_17)
target_include_directories(
  sturdy_guide_camera_starter
  PRIVATE ${OpenCV_INCLUDE_DIRS}
)
target_link_libraries(
  sturdy_guide_camera_starter
  PRIVATE ${OpenCV_LIBS}
)

if(STURDY_GUIDE_ENABLE_WARNINGS)
  if(MSVC)
    target_compile_options(sturdy_guide_camera_starter PRIVATE /W4)
  else()
    target_compile_options(
      sturdy_guide_camera_starter
      PRIVATE -Wall -Wextra -Wpedantic
    )
  endif()
endif()

set_target_properties(
  sturdy_guide_camera_starter
  PROPERTIES
    OUTPUT_NAME sturdy-guide-camera-starter
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
)
