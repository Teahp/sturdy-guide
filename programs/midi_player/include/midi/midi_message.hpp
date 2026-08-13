#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace sturdy_guide::midi {

// 一个最小的三字节 MIDI 1.0 Channel Voice 消息。
class MidiMessage {
 public:
  static MidiMessage note_on(std::uint8_t channel, std::uint8_t note,
                             std::uint8_t velocity);
  static MidiMessage note_off(std::uint8_t channel, std::uint8_t note,
                              std::uint8_t velocity = 0);

  [[nodiscard]] const std::array<std::uint8_t, 3>& bytes() const noexcept;
  [[nodiscard]] bool is_note_on() const noexcept;
  [[nodiscard]] std::uint8_t channel() const noexcept;
  [[nodiscard]] std::uint8_t note() const noexcept;
  [[nodiscard]] std::uint8_t velocity() const noexcept;

 private:
  explicit MidiMessage(std::array<std::uint8_t, 3> bytes);

  std::array<std::uint8_t, 3> bytes_{};
};

}  // namespace sturdy_guide::midi
