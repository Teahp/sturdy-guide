#pragma once

#include "midi/midi_output.hpp"

#include <filesystem>
#include <fstream>

namespace sturdy_guide::midi {

// Linux 原始 MIDI 后端。可将三字节消息写入 /dev/snd/midiC*D* 等设备。
class RawMidiOutput final : public MidiOutput {
 public:
  explicit RawMidiOutput(const std::filesystem::path& device);
  void send(const MidiMessage& message) override;

 private:
  std::ofstream stream_;
};

}  // namespace sturdy_guide::midi
