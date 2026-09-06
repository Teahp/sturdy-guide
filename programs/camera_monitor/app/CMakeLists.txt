add_executable(sturdy_guide_camera
    main.cpp
)

target_link_libraries(sturdy_guide_camera
    PRIVATE
        sturdy_guide_camera_core
)

target_compile_features(
    sturdy_guide_camera
    PRIVATE cxx_std_17
)

set_target_properties(sturdy_guide_camera
    PROPERTIES
        OUTPUT_NAME "sturdy-guide-camera"
)

if(MSVC)
    target_compile_options(sturdy_guide_camera
        PRIVATE /W4 /utf-8
    )
endif()