# 应用程序：参数解析、依赖组装和窗口循环
add_executable(sturdy-guide-camera
  main.cpp
  preview_application.cpp
)

target_include_directories(sturdy-guide-camera
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(sturdy-guide-camera
  PRIVATE
    camera_core
)

target_compile_features(sturdy-guide-camera PRIVATE cxx_std_17)

if(STURDY_GUIDE_ENABLE_WARNINGS)
  target_compile_options(sturdy-guide-camera PRIVATE -Wall -Wextra -Wpedantic)
endif()
