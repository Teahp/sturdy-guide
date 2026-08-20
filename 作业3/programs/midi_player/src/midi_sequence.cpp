#include "midi/midi_sequence.hpp"

#include <algorithm>
#include <utility>

namespace sturdy_guide::midi {

void MidiSequence::add(MidiEvent event) {
  // upper_bound 让相同时间的消息保持插入顺序，例如先 note_off 再 note_on。
  const auto position = std::upper_bound(
      events_.begin(), events_.end(), event.offset,
      [](const auto offset, const MidiEvent& existing) {
        return offset < existing.offset;
      });
  events_.insert(position, std::move(event));
}

const std::vector<MidiEvent>& MidiSequence::events() const noexcept {
  return events_;
}

bool MidiSequence::empty() const noexcept { return events_.empty(); }

}  // namespace sturdy_guide::midi
