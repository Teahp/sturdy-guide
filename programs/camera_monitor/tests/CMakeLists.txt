add_executable(camera_session_test camera_session_test.cpp)
target_link_libraries(camera_session_test PRIVATE SturdyGuide::camera_core)

if(STURDY_GUIDE_ENABLE_WARNINGS)
  if(MSVC)
    target_compile_options(camera_session_test PRIVATE /W4)
  else()
    target_compile_options(
      camera_session_test
      PRIVATE -Wall -Wextra -Wpedantic
    )
  endif()
endif()

add_test(NAME camera_session_test COMMAND camera_session_test)
