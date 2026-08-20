#pragma once

#include "midi/midi_output.hpp"

#include <iosfwd>
#include <mutex>

namespace sturdy_guide::midi {

// 教学用后端：打印 MIDI 消息。它不合成音频，但无需外部 SDK 即可运行。
class ConsoleMidiOutput final : public MidiOutput {
 public:
  explicit ConsoleMidiOutput(std::ostream& output);
  void send(const MidiMessage& message) override;

 private:
  std::ostream& output_;
  std::mutex output_mutex_;
};

}  // namespace sturdy_guide::midi
