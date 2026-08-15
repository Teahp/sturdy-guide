#pragma once

#include "midi/midi_message.hpp"

namespace sturdy_guide::midi {

// 输出接口隔离播放器与具体设备。实现类必须比调用 send() 的播放器活得更久。
class MidiOutput {
 public:
  virtual ~MidiOutput() = default;
  virtual void send(const MidiMessage& message) = 0;
};

}  // namespace sturdy_guide::midi
