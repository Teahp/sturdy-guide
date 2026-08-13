add_executable(sturdy_guide_midi_player main.cpp)
target_link_libraries(
  sturdy_guide_midi_player
  PRIVATE SturdyGuide::midi_core
)

set_target_properties(
  sturdy_guide_midi_player
  PROPERTIES
    OUTPUT_NAME sturdy-guide-midi
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
)
