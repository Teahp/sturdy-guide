# 应用 target：最终程序 sturdy-guide-camera，链接核心库。
add_executable(sturdy_guide_camera main.cpp)

target_link_libraries(sturdy_guide_camera PRIVATE sturdy_guide_camera_core)

target_compile_features(sturdy_guide_camera PRIVATE cxx_std_17)

set_target_properties(
  sturdy_guide_camera
  PROPERTIES
    OUTPUT_NAME sturdy-guide-camera
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
)

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
