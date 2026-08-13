find_package(Threads REQUIRED)

# 领域模型和播放器被组织为库，CLI 与测试只依赖公开接口。
add_library(sturdy_guide_midi_core
  src/console_midi_output.cpp
  src/demo_song.cpp
  src/midi_message.cpp
  src/midi_player.cpp
  src/midi_sequence.cpp
  src/raw_midi_output.cpp
)
add_library(SturdyGuide::midi_core ALIAS sturdy_guide_midi_core)

target_compile_features(sturdy_guide_midi_core PUBLIC cxx_std_17)
target_include_directories(
  sturdy_guide_midi_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# std::thread 的链接要求必须传递给使用这个静态库的可执行程序。
target_link_libraries(sturdy_guide_midi_core PUBLIC Threads::Threads)

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
