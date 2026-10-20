// CameraSession 的硬件无关测试：全部通过 FakeFrameSource 驱动，
// 不打开真实设备，也不创建窗口。
//
// 用法与仓库既有测试一致：所有用例跑完，用 0 / 1 向 CTest 报告结果。

#include "camera/camera_session.hpp"
#include "camera/fake_frame_source.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

using namespace std::chrono_literals;

int failures = 0;

void expect_true(const std::string& name, const bool condition,
                 const std::string& detail) {
  if (condition) {
    std::cout << "[ ok ] " << name << '\n';
    return;
  }
  ++failures;
  std::cerr << "[fail] " << name;
  if (!detail.empty()) {
    std::cerr << " -- " << detail;
  }
  std::cerr << '\n';
}

void expect_equal(const std::string& name, const std::uint64_t actual,
                  const std::uint64_t expected) {
  expect_true(name, actual == expected,
              "expected " + std::to_string(expected) + ", got " +
                  std::to_string(actual));
}

camera::FakeFrameSource::Config make_config(
    const std::size_t total_frames, const std::chrono::microseconds period) {
  camera::FakeFrameSource::Config config;
  config.total_frames = total_frames;
  config.period = period;
  return config;
}

// 一直取帧直到超时或会话停止，返回取到的帧数与其中的空帧数。
std::size_t drain(camera::CameraSession& session, std::size_t& empty_frames) {
  std::size_t received = 0;
  empty_frames = 0;
  for (;;) {
    camera::Frame frame;
    if (!session.wait_for_frame(frame, 200ms)) {
      return received;
    }
    ++received;
    if (frame.image.empty()) {
      ++empty_frames;
    }
  }
}

// 1) start() 之后能取到帧，且 sequence 递增。
void test_start_delivers_frames() {
  auto probe = std::make_shared<camera::SourceProbe>();
  auto source =
      std::make_unique<camera::FakeFrameSource>(make_config(0, 1ms), probe);
  camera::CameraSession session{std::move(source)};

  expect_true("start() succeeds for an openable device", session.start(), "");
  expect_equal("start() opened the device exactly once",
               static_cast<std::uint64_t>(probe->open_calls.load()), 1);

  bool increasing = true;
  bool all_non_empty = true;
  std::uint64_t previous = 0;
  std::size_t received = 0;
  while (received < 5) {
    camera::Frame frame;
    if (!session.wait_for_frame(frame, 2s)) {
      break;
    }
    if (frame.sequence <= previous) {
      increasing = false;
    }
    if (frame.image.empty()) {
      all_non_empty = false;
    }
    previous = frame.sequence;
    ++received;
  }

  expect_equal("five frames are delivered", received, 5);
  expect_true("sequence numbers strictly increase", increasing,
              "last sequence " + std::to_string(previous));
  expect_true("delivered frames are never empty", all_non_empty, "");
  expect_true("sequence numbering starts at one", previous >= 1, "");

  session.stop();
}

// 2) 重复 start() 的行为符合契约：抛 std::logic_error，且不影响已运行的会话。
void test_repeated_start_throws() {
  auto source = std::make_unique<camera::FakeFrameSource>(make_config(0, 1ms));
  camera::CameraSession session{std::move(source)};

  expect_true("first start() succeeds", session.start(), "");

  bool threw = false;
  try {
    (void)session.start();
  } catch (const std::logic_error&) {
    threw = true;
  }
  expect_true("second start() throws std::logic_error", threw, "");
  expect_true("session is still running after the rejected start()",
              session.is_running(), "");

  camera::Frame frame;
  expect_true("frames are still available after the rejected start()",
              session.wait_for_frame(frame, 2s), "");

  session.stop();
}

// 3) 连续 stop() 是安全的：不崩溃、不死锁，未启动时也能调用。
void test_repeated_stop_is_safe() {
  auto probe = std::make_shared<camera::SourceProbe>();
  auto source =
      std::make_unique<camera::FakeFrameSource>(make_config(0, 1ms), probe);
  camera::CameraSession session{std::move(source)};

  session.stop();
  session.stop();
  expect_true("stop() before start() is harmless", !session.is_running(), "");

  expect_true("start() still works after an early stop()", session.start(), "");
  session.stop();
  session.stop();
  expect_true("repeated stop() leaves the session idle", !session.is_running(),
              "");
  expect_true("device is closed after stop()", probe->close_calls.load() >= 1,
              std::to_string(probe->close_calls.load()));

  // 停止后可以再次启动：状态机允许 Idle -> Running -> Idle -> Running。
  expect_true("session can be started again after stop()", session.start(), "");
  camera::Frame frame;
  expect_true("restarted session delivers frames",
              session.wait_for_frame(frame, 2s), "");
  session.stop();
}

// 4) 析构时工作线程已经 join，设备对象于是在读帧之外被销毁。
void test_worker_joined_before_destruction() {
  auto probe = std::make_shared<camera::SourceProbe>();
  std::size_t reads_while_running = 0;
  {
    auto source =
        std::make_unique<camera::FakeFrameSource>(make_config(0, 5ms), probe);
    camera::CameraSession session{std::move(source)};
    (void)session.start();

    camera::Frame frame;
    (void)session.wait_for_frame(frame, 2s);
    reads_while_running = static_cast<std::size_t>(probe->reads.load());
  }

  expect_true("worker is reading while the session runs",
              reads_while_running > 0, std::to_string(reads_while_running));
  expect_true("no read is in flight after destruction",
              probe->reads_in_flight.load() == 0,
              std::to_string(probe->reads_in_flight.load()));
  expect_true("frame source is destroyed with the session",
              probe->destroyed.load(), "");
  expect_true("device was closed by the destructor",
              probe->close_calls.load() >= 1,
              std::to_string(probe->close_calls.load()));
}

// 5) 缓冲始终不超过两帧，溢出时丢弃最旧的一帧。
void test_buffer_stays_bounded() {
  auto source = std::make_unique<camera::FakeFrameSource>(make_config(0, 1ms));
  camera::CameraSession session{std::move(source)};
  (void)session.start();

  // 消费者故意什么都不做，让采集线程把缓冲填满并开始丢帧。
  std::this_thread::sleep_for(300ms);
  session.stop();

  const camera::CameraSession::Stats stats = session.stats();
  expect_true("buffer never exceeds two frames",
              stats.buffered <= camera::CameraSession::kMaxBufferedFrames,
              std::to_string(stats.buffered));
  expect_true("captured enough frames for the test to be meaningful",
              stats.captured > 10, std::to_string(stats.captured));
  expect_true("old frames are dropped once the buffer is full",
              stats.dropped > 0, std::to_string(stats.dropped));
  expect_equal("delivered + dropped + buffered equals captured",
               stats.delivered + stats.dropped + stats.buffered,
               stats.captured);

  // 缓冲里剩下的必须是最新的一帧：停止后取出的第一帧就是最后采集的那一帧。
  camera::Frame newest;
  expect_true("the newest frame is still available after stop()",
              session.wait_for_frame(newest, 1s), "");

  const std::uint64_t oldest_kept = stats.captured - stats.buffered;
  expect_true("the dropped frames were the oldest ones",
              newest.sequence >= oldest_kept,
              "sequence " + std::to_string(newest.sequence) + " < " +
                  std::to_string(oldest_kept));
}

// 6) 后台读帧异常能被主线程观察到，并且异常信息原样穿过线程边界。
void test_read_error_is_observable() {
  camera::FakeFrameSource::Config config = make_config(0, 1ms);
  config.throw_at = 3;

  auto source = std::make_unique<camera::FakeFrameSource>(config);
  camera::CameraSession session{std::move(source)};
  (void)session.start();

  bool saw_error = false;
  for (int attempt = 0; attempt < 200 && !saw_error; ++attempt) {
    camera::Frame frame;
    (void)session.wait_for_frame(frame, 20ms);
    saw_error = session.has_error();
  }
  expect_true("has_error() becomes true after the device throws", saw_error,
              "");
  expect_true("session reports itself as stopped after the failure",
              !session.is_running(), "");

  std::string message;
  const std::exception_ptr error = session.take_error();
  try {
    std::rethrow_exception(error);
  } catch (const std::exception& exception) {
    message = exception.what();
  } catch (...) {
    message = "<non-standard exception>";
  }
  expect_true("error message survives the thread boundary",
              message.find("simulated read failure") != std::string::npos,
              message);
  expect_true("take_error() clears the stored error",
              session.take_error() == nullptr, "");

  session.stop();
}

// 7) 空帧不会被当成有效画面发布，但会被计数。
void test_empty_frames_are_not_published() {
  camera::FakeFrameSource::Config config = make_config(12, 1ms);
  config.empty_at = 4;

  auto source = std::make_unique<camera::FakeFrameSource>(config);
  camera::CameraSession session{std::move(source)};
  (void)session.start();

  std::size_t empty_frames = 0;
  const std::size_t received = drain(session, empty_frames);
  const camera::CameraSession::Stats stats = session.stats();

  expect_equal("no empty frame is ever delivered", empty_frames, 0);
  expect_equal("the simulated empty frame is counted", stats.empty, 1);
  // 12 次读取里有 1 次是空帧，因此最多发布 11 帧；丢帧时 delivered 会少于 11。
  expect_equal("every published frame is delivered, dropped, or buffered",
               received + stats.dropped + stats.buffered, 11);

  session.stop();
}

}  // namespace

int main() {
  test_start_delivers_frames();
  test_repeated_start_throws();
  test_repeated_stop_is_safe();
  test_worker_joined_before_destruction();
  test_buffer_stays_bounded();
  test_read_error_is_observable();
  test_empty_frames_are_not_published();

  if (failures == 0) {
    std::cout << "camera_session_test: all cases passed\n";
    return 0;
  }
  std::cerr << "camera_session_test: " << failures << " case(s) failed\n";
  return 1;
}
