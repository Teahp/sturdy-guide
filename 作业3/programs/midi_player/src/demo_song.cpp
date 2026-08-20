#include "midi/demo_song.hpp"

#include <array>
#include <chrono>
#include <cstdint>

namespace sturdy_guide::midi {

MidiSequence make_demo_scale() {
  using namespace std::chrono_literals;

  constexpr std::array<std::uint8_t, 8> notes{60, 62, 64, 65,
                                               67, 69, 71, 72};
  constexpr auto note_length = 220ms;

  MidiSequence sequence;
  auto offset = 0ms;
  for (const auto note : notes) {
    sequence.add({offset, MidiMessage::note_on(0, note, 96)});
    sequence.add({offset + note_length, MidiMessage::note_off(0, note)});
    offset += note_length;
  }
  return sequence;
}

}  // namespace sturdy_guide::midi
