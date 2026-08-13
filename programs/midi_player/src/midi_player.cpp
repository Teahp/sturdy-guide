#include "midi/midi_player.hpp"

#include <chrono>
#include <exception>
#include <stdexcept>
#include <utility>

namespace sturdy_guide::midi {

MidiPlayer::MidiPlayer(MidiOutput& output) : output_(output) {}

MidiPlayer::~MidiPlayer() {
  // 先让等待中的线程退出，再 join，保证线程不会访问已经析构的成员。
  stop();
  try {
    wait();
  } catch (...) {
    // 析构函数不能把后台错误继续抛出；显式 wait() 才是观察错误的边界。
  }
}

void MidiPlayer::play(MidiSequence sequence) {
  std::thread completed_worker;
  {
    std::scoped_lock lock{mutex_};
    if (playing_) {
      throw std::logic_error("the MIDI player is already playing");
    }

    // 上一次自然结束的线程仍然需要 join。先移动出来，避免持锁 join。
    if (worker_.joinable()) {
      completed_worker = std::move(worker_);
    }
    stop_requested_ = false;
    playback_error_ = nullptr;
    playing_ = true;
  }

  if (completed_worker.joinable()) {
    completed_worker.join();
  }

  try {
    std::scoped_lock lock{mutex_};
    worker_ = std::thread(&MidiPlayer::run, this, std::move(sequence));
  } catch (...) {
    std::scoped_lock lock{mutex_};
    playing_ = false;
    throw;
  }
}

void MidiPlayer::stop() {
  {
    const std::scoped_lock lock{mutex_};
    stop_requested_ = true;
  }
  state_changed_.notify_all();
}

void MidiPlayer::wait() {
  std::thread worker;
  {
    std::scoped_lock lock{mutex_};
    if (worker_.joinable()) {
      worker = std::move(worker_);
    }
  }
  if (worker.joinable()) {
    worker.join();
  }

  std::exception_ptr playback_error;
  {
    const std::scoped_lock lock{mutex_};
    playback_error = std::exchange(playback_error_, nullptr);
  }
  if (playback_error) {
    std::rethrow_exception(playback_error);
  }
}

bool MidiPlayer::is_playing() const {
  const std::scoped_lock lock{mutex_};
  return playing_;
}

void MidiPlayer::run(MidiSequence sequence) {
  const auto started_at = std::chrono::steady_clock::now();

  try {
    for (const MidiEvent& event : sequence.events()) {
      std::unique_lock lock{mutex_};
      const auto interrupted = state_changed_.wait_until(
          lock, started_at + event.offset,
          [this] { return stop_requested_; });
      if (interrupted) {
        break;
      }

      // 不在持锁状态下调用外部多态对象，避免阻塞播放器的 stop()。
      lock.unlock();
      output_.send(event.message);
    }
  } catch (...) {
    const std::scoped_lock lock{mutex_};
    playback_error_ = std::current_exception();
  }

  {
    const std::scoped_lock lock{mutex_};
    playing_ = false;
  }
  state_changed_.notify_all();
}

}  // namespace sturdy_guide::midi
