add_executable(sturdy_guide_camera main.cpp preview_application.cpp)
target_link_libraries(sturdy_guide_camera PRIVATE SturdyGuide::camera_core)

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
