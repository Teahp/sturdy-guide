find_package(Threads REQUIRED)

# 领域模型和播放器被组织为库，CLI 与测试只依赖公开接口。
set(sturdy_guide_midi_sources
  src/console_midi_output.cpp
  src/demo_song.cpp
  src/midi_message.cpp
  src/midi_player.cpp
  src/midi_sequence.cpp
)

# 平台设备后端只在对应系统编译，公共播放器逻辑保持不变。
if(WIN32)
  list(APPEND sturdy_guide_midi_sources src/win_mm_midi_output.cpp)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  list(APPEND sturdy_guide_midi_sources src/raw_midi_output.cpp)
endif()

add_library(sturdy_guide_midi_core ${sturdy_guide_midi_sources})
add_library(SturdyGuide::midi_core ALIAS sturdy_guide_midi_core)

target_compile_features(sturdy_guide_midi_core PUBLIC cxx_std_17)
target_include_directories(
  sturdy_guide_midi_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# std::thread 的链接要求必须传递给使用这个静态库的可执行程序。
target_link_libraries(sturdy_guide_midi_core PUBLIC Threads::Threads)

# WinMM 属于 Windows SDK；链接系统库即可，无需安装第三方 MIDI SDK。
if(WIN32)
  target_link_libraries(sturdy_guide_midi_core PRIVATE winmm)
endif()

if(STURDY_GUIDE_ENABLE_WARNINGS)
  if(MSVC)
    target_compile_options(sturdy_guide_midi_core PRIVATE /W4)
  else()
    target_compile_options(
      sturdy_guide_midi_core
      PRIVATE -Wall -Wextra -Wpedantic
    )
  endif()
endif()

add_subdirectory(app)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
