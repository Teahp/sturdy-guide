# 这是供学生重构的单文件起始 target，仅供对照，默认不构建。
# 正式 target 是 app/sturdy_guide_camera（输出名 sturdy-guide-camera），
# 因此这里的输出名改为 -starter 后缀，避免两个可执行文件冲突。
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
