# 核心库：设备抽象和会话管理
add_library(camera_core
  opencv_camera.cpp
  camera_session.cpp
)

target_include_directories(camera_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(camera_core
  PUBLIC
    ${OpenCV_LIBS}
    Threads::Threads
)

target_compile_features(camera_core PUBLIC cxx_std_17)

if(STURDY_GUIDE_ENABLE_WARNINGS)
  target_compile_options(camera_core PRIVATE -Wall -Wextra -Wpedantic)
endif()
