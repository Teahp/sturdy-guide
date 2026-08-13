#include "midi/midi_message.hpp"

#include <stdexcept>
#include <string>

namespace sturdy_guide::midi {
namespace {

void validate_channel(const std::uint8_t channel) {
  if (channel > 15) {
    throw std::out_of_range("MIDI channel must be between 0 and 15");
  }
}

void validate_data_byte(const std::uint8_t value, const char* const name) {
  if (value > 127) {
    throw std::out_of_range(std::string{name} +
                            " must be between 0 and 127");
  }
}

}  // namespace

MidiMessage MidiMessage::note_on(const std::uint8_t channel,
                                 const std::uint8_t note,
                                 const std::uint8_t velocity) {
  validate_channel(channel);
  validate_data_byte(note, "MIDI note");
  validate_data_byte(velocity, "MIDI velocity");
  return MidiMessage{{static_cast<std::uint8_t>(0x90U | channel), note,
                      velocity}};
}

MidiMessage MidiMessage::note_off(const std::uint8_t channel,
                                  const std::uint8_t note,
                                  const std::uint8_t velocity) {
  validate_channel(channel);
  validate_data_byte(note, "MIDI note");
  validate_data_byte(velocity, "MIDI velocity");
  return MidiMessage{{static_cast<std::uint8_t>(0x80U | channel), note,
                      velocity}};
}

MidiMessage::MidiMessage(const std::array<std::uint8_t, 3> bytes)
    : bytes_(bytes) {}

const std::array<std::uint8_t, 3>& MidiMessage::bytes() const noexcept {
  return bytes_;
}

bool MidiMessage::is_note_on() const noexcept {
  return (bytes_[0] & 0xF0U) == 0x90U && velocity() != 0;
}

std::uint8_t MidiMessage::channel() const noexcept { return bytes_[0] & 0x0FU; }

std::uint8_t MidiMessage::note() const noexcept { return bytes_[1]; }

std::uint8_t MidiMessage::velocity() const noexcept { return bytes_[2]; }

}  // namespace sturdy_guide::midi
