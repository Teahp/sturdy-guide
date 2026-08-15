#pragma once

#include "midi/midi_output.hpp"
#include "midi/midi_sequence.hpp"

#include <condition_variable>
#include <exception>
#include <mutex>
#include <thread>

namespace sturdy_guide::midi {

// MidiPlayer 独占一个工作线程；同一时刻最多播放一条序列。
// play() 和 wait() 由同一个控制线程调用；stop() 与 is_playing()
// 可以从其他线程调用。
class MidiPlayer {
 public:
  explicit MidiPlayer(MidiOutput& output);
  ~MidiPlayer();

  MidiPlayer(const MidiPlayer&) = delete;
  MidiPlayer& operator=(const MidiPlayer&) = delete;

  // play() 启动后台播放。播放器忙时抛出 std::logic_error。
  void play(MidiSequence sequence);

  // stop() 可被重复调用，并中断当前等待。
  void stop();

  // 等待当前播放结束；没有播放任务时立即返回。
  void wait();

  [[nodiscard]] bool is_playing() const;

 private:
  void run(MidiSequence sequence);

  MidiOutput& output_;
  mutable std::mutex mutex_;
  std::condition_variable state_changed_;
  std::thread worker_;
  std::exception_ptr playback_error_;
  bool stop_requested_ = false;
  bool playing_ = false;
};

}  // namespace sturdy_guide::midi
