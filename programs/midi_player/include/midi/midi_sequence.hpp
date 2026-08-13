#pragma once

#include "midi/midi_event.hpp"

#include <vector>

namespace sturdy_guide::midi {

// MidiSequence 保证事件按时间排序，播放器无需处理无序输入。
class MidiSequence {
 public:
  void add(MidiEvent event);

  [[nodiscard]] const std::vector<MidiEvent>& events() const noexcept;
  [[nodiscard]] bool empty() const noexcept;

 private:
  std::vector<MidiEvent> events_;
};

}  // namespace sturdy_guide::midi
