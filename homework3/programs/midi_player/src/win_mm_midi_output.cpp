#include "midi/win_mm_midi_output.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>

#include <array>
#include <stdexcept>
#include <string>
#include <utility>

namespace sturdy_guide::midi {
namespace {

std::string utf8_from_wide(const wchar_t* const text) {
  if (text == nullptr || text[0] == L'\0') {
    return {};
  }

  const int size = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0,
                                       nullptr, nullptr);
  if (size <= 1) {
    return "unknown";
  }

  // size 包含结尾的空字符；转换完成后再去掉它。
  std::string result(static_cast<std::size_t>(size), '\0');
  const int converted = WideCharToMultiByte(CP_UTF8, 0, text, -1,
                                             result.data(), size, nullptr,
                                             nullptr);
  if (converted != size) {
    return "unknown";
  }
  result.pop_back();
  return result;
}

std::runtime_error make_win_mm_error(const char* const operation,
                                     const MMRESULT result) {
  std::array<wchar_t, MAXERRORLENGTH> buffer{};
  if (midiOutGetErrorTextW(result, buffer.data(),
                           static_cast<UINT>(buffer.size())) ==
      MMSYSERR_NOERROR) {
    return std::runtime_error(std::string{operation} + ": " +
                              utf8_from_wide(buffer.data()));
  }
  return std::runtime_error(std::string{operation} +
                            " failed with WinMM code " +
                            std::to_string(result));
}

}  // namespace

class WinMmMidiOutput::Impl {
 public:
  explicit Impl(const std::uint32_t device_id) {
    const MMRESULT result =
        midiOutOpen(&handle_, static_cast<UINT>(device_id), 0, 0, CALLBACK_NULL);
    if (result != MMSYSERR_NOERROR) {
      throw make_win_mm_error("cannot open MIDI output device", result);
    }
  }

  ~Impl() {
    if (handle_ != nullptr) {
      // reset 会立即关闭仍在发声的音符，避免异常退出后出现长音。
      midiOutReset(handle_);
      midiOutClose(handle_);
    }
  }

  void send(const MidiMessage& message) {
    const auto& bytes = message.bytes();
    const DWORD packed_message =
        static_cast<DWORD>(bytes[0]) |
        (static_cast<DWORD>(bytes[1]) << 8U) |
        (static_cast<DWORD>(bytes[2]) << 16U);

    const MMRESULT result = midiOutShortMsg(handle_, packed_message);
    if (result != MMSYSERR_NOERROR) {
      throw make_win_mm_error("failed to send MIDI message", result);
    }
  }

 private:
  HMIDIOUT handle_ = nullptr;
};

WinMmMidiOutput::WinMmMidiOutput(const std::uint32_t device_id)
    : impl_(std::make_unique<Impl>(device_id)) {}

WinMmMidiOutput::~WinMmMidiOutput() = default;
WinMmMidiOutput::WinMmMidiOutput(WinMmMidiOutput&&) noexcept = default;
WinMmMidiOutput& WinMmMidiOutput::operator=(WinMmMidiOutput&&) noexcept =
    default;

void WinMmMidiOutput::send(const MidiMessage& message) {
  if (!impl_) {
    throw std::logic_error("cannot use a moved-from WinMmMidiOutput");
  }
  impl_->send(message);
}

std::vector<std::string> WinMmMidiOutput::devices() {
  std::vector<std::string> result;
  const UINT count = midiOutGetNumDevs();
  result.reserve(count);

  for (UINT id = 0; id < count; ++id) {
    MIDIOUTCAPSW capabilities{};
    const MMRESULT status = midiOutGetDevCapsW(
        id, &capabilities, static_cast<UINT>(sizeof(capabilities)));
    if (status != MMSYSERR_NOERROR) {
      throw make_win_mm_error("cannot query MIDI output device", status);
    }
    result.push_back(utf8_from_wide(capabilities.szPname));
  }
  return result;
}

}  // namespace sturdy_guide::midi
