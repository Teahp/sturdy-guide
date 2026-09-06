add_executable(sturdy_guide_camera_tests
    camera_session_test.cpp
)

target_link_libraries(sturdy_guide_camera_tests
    PRIVATE
        sturdy_guide_camera_core
)

target_compile_features(
    sturdy_guide_camera_tests
    PRIVATE cxx_std_17
)

if(MSVC)
    target_compile_options(sturdy_guide_camera_tests
        PRIVATE /W4 /utf-8
    )
endif()

add_test(
    NAME sturdy_guide_camera_tests
    COMMAND sturdy_guide_camera_tests
)
