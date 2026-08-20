#include "midi/console_midi_output.hpp"

#include <iomanip>
#include <ostream>

namespace sturdy_guide::midi {

ConsoleMidiOutput::ConsoleMidiOutput(std::ostream& output) : output_(output) {}

void ConsoleMidiOutput::send(const MidiMessage& message) {
  const std::scoped_lock lock{output_mutex_};
  output_ << (message.is_note_on() ? "NOTE ON " : "NOTE OFF")
          << " channel=" << static_cast<unsigned>(message.channel())
          << " note=" << static_cast<unsigned>(message.note())
          << " velocity=" << static_cast<unsigned>(message.velocity()) << "  [";

  for (const auto byte : message.bytes()) {
    output_ << " 0x" << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<unsigned>(byte);
  }
  output_ << std::dec << std::setfill(' ') << " ]\n";
}

}  // namespace sturdy_guide::midi
