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

add_library(sturdy_guide
  src/greeting.cpp
  src/text.cpp
)
add_library(SturdyGuide::sturdy_guide ALIAS sturdy_guide)

target_compile_features(sturdy_guide PUBLIC cxx_std_17)
target_include_directories(
  sturdy_guide
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(STURDY_GUIDE_ENABLE_WARNINGS)
  if(MSVC)
    target_compile_options(sturdy_guide PRIVATE /W4)
  else()
    target_compile_options(sturdy_guide PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endif()

add_executable(sturdy_guide_cli app/main.cpp)
set_target_properties(sturdy_guide_cli PROPERTIES OUTPUT_NAME sturdy-guide)
target_link_libraries(sturdy_guide_cli PRIVATE SturdyGuide::sturdy_guide)

if(BUILD_TESTING)
  function(add_sturdy_guide_test target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE SturdyGuide::sturdy_guide)
    add_test(NAME ${target} COMMAND ${target})
  endfunction()

  add_sturdy_guide_test(greeting_test tests/greeting_test.cpp)
  add_sturdy_guide_test(text_test tests/text_test.cpp)
endif()

install(
  TARGETS sturdy_guide
  EXPORT SturdyGuideTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

set(sturdy_guide_cmake_dir "${CMAKE_INSTALL_LIBDIR}/cmake/SturdyGuide")

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

