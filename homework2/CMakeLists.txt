cmake_minimum_required(VERSION 3.16)

project(
  SturdyGuide
  VERSION 0.1.0
  DESCRIPTION "A small multi-file C++ SDK used for Linux, CMake, CI, and Git lessons"
  LANGUAGES CXX
)

include(CTest)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(STURDY_GUIDE_ENABLE_WARNINGS "Enable common compiler warnings" ON)

# 根文件只负责编排项目；每个子目录负责声明自己的 target。
add_subdirectory(src)
add_subdirectory(app)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

# 公共头文件不仅供源码构建使用，也是可安装 SDK 的一部分。
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

set(sturdy_guide_cmake_dir "${CMAKE_INSTALL_LIBDIR}/cmake/SturdyGuide")

# 生成包配置，让外部项目能通过 find_package(SturdyGuide) 找到 SDK。
configure_package_config_file(
  cmake/SturdyGuideConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/SturdyGuideConfig.cmake"
  INSTALL_DESTINATION "${sturdy_guide_cmake_dir}"
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/SturdyGuideConfigVersion.cmake"
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY SameMajorVersion
)

install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/SturdyGuideConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/SturdyGuideConfigVersion.cmake"
  DESTINATION "${sturdy_guide_cmake_dir}"
)
install(
  EXPORT SturdyGuideTargets
  FILE SturdyGuideTargets.cmake
  NAMESPACE SturdyGuide::
  DESTINATION "${sturdy_guide_cmake_dir}"
)
