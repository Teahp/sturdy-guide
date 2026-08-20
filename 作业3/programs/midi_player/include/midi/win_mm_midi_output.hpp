#pragma once

#include "midi/midi_output.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace sturdy_guide::midi {

// Windows WinMM 后端。实现细节藏在 Impl 中，公共头文件无需包含 windows.h。
class WinMmMidiOutput final : public MidiOutput {
 public:
  explicit WinMmMidiOutput(std::uint32_t device_id);
  ~WinMmMidiOutput() override;

  WinMmMidiOutput(const WinMmMidiOutput&) = delete;
  WinMmMidiOutput& operator=(const WinMmMidiOutput&) = delete;
  WinMmMidiOutput(WinMmMidiOutput&&) noexcept;
  WinMmMidiOutput& operator=(WinMmMidiOutput&&) noexcept;

  void send(const MidiMessage& message) override;

  // 返回值的下标就是传给构造函数的 WinMM 设备编号。
  [[nodiscard]] static std::vector<std::string> devices();

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace sturdy_guide::midi
