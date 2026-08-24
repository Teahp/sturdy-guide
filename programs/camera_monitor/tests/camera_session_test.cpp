#include "camera/camera_session.hpp"
#include "fake_frame_source.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <thread>

namespace {

using sturdy_guide::camera::CameraSession;
using sturdy_guide::camera::FakeFrameSource;
using sturdy_guide::camera::Frame;

// 自动测试绝不开真实设备：设备编号被 FakeFrameSource 忽略。
constexpr int kFakeDevice = 0;

bool expect(const std::string_view name, const bool condition) {
  if (condition) return true;
  std::cerr << name << " failed\n";
  return false;
}

// 轮询等待一帧，或超时返回 false。
bool wait_for_frame(CameraSession& session, Frame& out,
                    const std::chrono::milliseconds timeout =
                        std::chrono::seconds{5}) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    if (session.tryTakeFrame(out)) return true;
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
  }
  return false;
}

// 1. start() 后能取得带递增序号的帧。
bool test_frames_have_increasing_sequence() {
  CameraSession session{std::make_unique<FakeFrameSource>()};
  session.start(kFakeDevice, 320, 240);

  Frame first;
  Frame second;
  if (!wait_for_frame(session, first)) return expect("received first frame", false);
  if (!wait_for_frame(session, second)) return expect("received second frame", false);

  session.stop();
  return expect("sequence increases", second.sequence > first.sequence);
}

// 2. 重复 start() 是 no-op（不会再次 open 设备）。
bool test_repeated_start_is_noop() {
  auto fake = std::make_unique<FakeFrameSource>();
  FakeFrameSource* raw = fake.get();
  CameraSession session{std::move(fake)};

  session.start(kFakeDevice, 320, 240);
  session.start(kFakeDevice, 320, 240);  // 已运行：应 no-op

  const bool opened_once = raw->open_calls() == 1;
  session.stop();
  return expect("open called exactly once", opened_once);
}

// 3. 连续调用 stop() 安全（不崩溃、不死锁）。
bool test_repeated_stop_is_safe() {
  CameraSession session{std::make_unique<FakeFrameSource>()};
  session.stop();  // 未 start 时 stop 也应是 no-op
  session.start(kFakeDevice, 320, 240);
  session.stop();
  session.stop();
  session.stop();
  return expect("repeated stop did not crash or deadlock", true);
}

// 4. 对象析构前工作线程已被 join。
bool test_destructor_joins_worker() {
  FakeFrameSource::Options options;
  options.delay = std::chrono::milliseconds{50};
  auto fake = std::make_unique<FakeFrameSource>(options);
  FakeFrameSource* raw = fake.get();

  {
    CameraSession session{std::move(fake)};
    session.start(kFakeDevice, 320, 240);
    // 等 worker 进入 read()（正处于 50ms 的睡眠中），确保销毁时它在临界态。
    while (raw->reads_in_flight() == 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
  }  // 析构 -> stop() -> join() 必须等待 worker 完成当前 read 并退出

  return expect("worker joined before destruction returned",
                raw->reads_in_flight() == 0);
}

// 5. 消费慢时缓冲有界并丢旧帧（只保留最新帧，无积压队列）。
bool test_bounded_buffer_drops_old_frames() {
  FakeFrameSource::Options options;
  options.delay = std::chrono::milliseconds{1};
  auto fake = std::make_unique<FakeFrameSource>(options);
  FakeFrameSource* raw = fake.get();
  CameraSession session{std::move(fake)};
  session.start(kFakeDevice, 320, 240);

  // 消费者不消费，让 worker 先产出大量帧。
  while (raw->produced() < 50) {
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
  }
  session.stop();

  Frame latest;
  if (!session.tryTakeFrame(latest)) {
    return expect("buffer retains newest frame", false);
  }
  // worker 在最后一次发布后还会多做一次未被发布的 read，且最新已发布
  // 那一帧本身也占一次 produced，因此最终 produced == 最新已发布序号 + 2。
  const bool newest_retained = latest.sequence + 2 == raw->produced();

  Frame extra;
  const bool no_backlog = !session.tryTakeFrame(extra);  // 无积压队列

  return expect("newest frame retained (old dropped)", newest_retained) &&
         expect("buffer bounded (no queued backlog)", no_backlog);
}

// 6. 假设备抛出的读取错误能在主线程被观察。
bool test_background_error_is_observable() {
  FakeFrameSource::Options options;
  options.throw_after = 5;
  CameraSession session{std::make_unique<FakeFrameSource>(options)};
  session.start(kFakeDevice, 320, 240);

  bool observed = false;
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{5};
  while (std::chrono::steady_clock::now() < deadline) {
    try {
      session.rethrowError();
    } catch (const std::runtime_error& error) {
      observed = std::string_view{error.what()} == "simulated capture failure";
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
  }
  session.stop();
  return expect("background error observed in main thread", observed);
}

// 7. 空帧不会被发布为有效画面。
bool test_empty_frames_are_skipped() {
  FakeFrameSource::Options options;
  options.empty_every = 2;  // 偶数序号为空帧，奇数序号为有效帧
  CameraSession session{std::make_unique<FakeFrameSource>(options)};
  session.start(kFakeDevice, 320, 240);

  int received = 0;
  bool saw_valid_only = true;
  Frame frame;
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{5};
  while (std::chrono::steady_clock::now() < deadline && received < 5) {
    if (session.tryTakeFrame(frame)) {
      ++received;
      if (frame.image.empty()) {
        saw_valid_only = false;
        break;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
  }
  session.stop();
  return expect("no empty frame published", saw_valid_only) &&
         expect("received enough valid frames", received >= 5);
}

}  // namespace

int main() {
  bool passed = true;
  passed &= test_frames_have_increasing_sequence();
  passed &= test_repeated_start_is_noop();
  passed &= test_repeated_stop_is_safe();
  passed &= test_destructor_joins_worker();
  passed &= test_bounded_buffer_drops_old_frames();
  passed &= test_background_error_is_observable();
  passed &= test_empty_frames_are_skipped();
  return passed ? 0 : 1;
}
