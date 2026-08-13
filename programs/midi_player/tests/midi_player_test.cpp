#include "midi/midi_message.hpp"
#include "midi/midi_output.hpp"
#include "midi/midi_player.hpp"
#include "midi/midi_sequence.hpp"

#include <chrono>
#include <cstddef>
#include <exception>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using sturdy_guide::midi::MidiMessage;

class RecordingOutput final : public sturdy_guide::midi::MidiOutput {
 public:
  void send(const MidiMessage& message) override {
    const std::scoped_lock lock{mutex_};
    messages_.push_back(message);
  }

  [[nodiscard]] std::vector<MidiMessage> messages() const {
    const std::scoped_lock lock{mutex_};
    return messages_;
  }

 private:
  mutable std::mutex mutex_;
  std::vector<MidiMessage> messages_;
};

class FailingOutput final : public sturdy_guide::midi::MidiOutput {
 public:
  void send(const MidiMessage&) override {
    throw std::runtime_error("simulated device failure");
  }
};

bool expect(const std::string_view name, const bool condition) {
  if (condition) {
    return true;
  }
  std::cerr << name << " failed\n";
  return false;
}

bool test_message_validation() {
  bool rejected_invalid_channel = false;
  try {
    static_cast<void>(MidiMessage::note_on(16, 60, 100));
  } catch (const std::out_of_range&) {
    rejected_invalid_channel = true;
  }

  const auto message = MidiMessage::note_on(2, 64, 90);
  return expect("note-on classification", message.is_note_on()) &&
         expect("channel value", message.channel() == 2) &&
         expect("note value", message.note() == 64) &&
         expect("invalid channel", rejected_invalid_channel);
}

bool test_sequence_order_and_playback() {
  using namespace std::chrono_literals;

  sturdy_guide::midi::MidiSequence sequence;
  sequence.add({2ms, MidiMessage::note_off(0, 60)});
  sequence.add({0ms, MidiMessage::note_on(0, 60, 100)});

  RecordingOutput output;
  sturdy_guide::midi::MidiPlayer player{output};
  player.play(std::move(sequence));
  player.wait();

  const auto messages = output.messages();
  return expect("two messages sent", messages.size() == 2) &&
         expect("messages sorted by offset", messages.front().is_note_on()) &&
         expect("player becomes idle", !player.is_playing());
}

bool test_stop_interrupts_wait() {
  using namespace std::chrono_literals;

  sturdy_guide::midi::MidiSequence sequence;
  sequence.add({10s, MidiMessage::note_on(0, 60, 100)});

  RecordingOutput output;
  sturdy_guide::midi::MidiPlayer player{output};
  player.play(std::move(sequence));
  player.stop();
  player.wait();

  return expect("stopped sequence sends nothing", output.messages().empty()) &&
         expect("stopped player becomes idle", !player.is_playing());
}

bool test_device_error_crosses_thread_boundary() {
  sturdy_guide::midi::MidiSequence sequence;
  sequence.add({{}, MidiMessage::note_on(0, 60, 100)});

  FailingOutput output;
  sturdy_guide::midi::MidiPlayer player{output};
  player.play(std::move(sequence));

  try {
    player.wait();
  } catch (const std::runtime_error& error) {
    return expect("device error message",
                  std::string_view{error.what()} == "simulated device failure");
  }
  return expect("device error propagated", false);
}

}  // namespace

int main() {
  bool passed = true;
  passed &= test_message_validation();
  passed &= test_sequence_order_and_playback();
  passed &= test_stop_interrupts_wait();
  passed &= test_device_error_crosses_thread_boundary();
  return passed ? 0 : 1;
}
