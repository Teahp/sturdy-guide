#pragma once

#include "midi/midi_message.hpp"

#include <chrono>

namespace sturdy_guide::midi {

// offset 是相对曲目起点的时间，而不是相邻事件之间的等待时长。
struct MidiEvent {
  std::chrono::milliseconds offset{};
  MidiMessage message;
};

}  // namespace sturdy_guide::midi
