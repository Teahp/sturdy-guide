#include "midi/raw_midi_output.hpp"

#include <stdexcept>

namespace sturdy_guide::midi {

RawMidiOutput::RawMidiOutput(const std::filesystem::path& device)
    : stream_(device, std::ios::binary | std::ios::out) {
  if (!stream_) {
    throw std::runtime_error("cannot open MIDI output device: " +
                             device.string());
  }
}

void RawMidiOutput::send(const MidiMessage& message) {
  const auto& bytes = message.bytes();
  stream_.write(reinterpret_cast<const char*>(bytes.data()),
                static_cast<std::streamsize>(bytes.size()));
  stream_.flush();
  if (!stream_) {
    throw std::runtime_error("failed to write MIDI message");
  }
}

}  // namespace sturdy_guide::midi
