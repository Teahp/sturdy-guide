#pragma once

#include "midi/midi_sequence.hpp"

namespace sturdy_guide::midi {

// 生成一段无需读取文件的 C 大调音阶，便于课堂演示播放器结构。
MidiSequence make_demo_scale();

}  // namespace sturdy_guide::midi
