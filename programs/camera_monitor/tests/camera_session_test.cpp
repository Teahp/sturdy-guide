#include "camera/camera_session.hpp"
#include "camera/fake_frame_source.hpp"

#include <opencv2/core.hpp>

#include <chrono>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <thread>

namespace {

using namespace std::chrono_literals;

using camera::CameraSession;
using camera::FakeFrameSource;

// 这个小项目不引入测试框架，用返回值向 CTest 报告成功或失败。
bool expect(const std::string_view name, const bool condition) {
  if (condition) {
    return true;
  }
  std::cerr << name << " failed\n";
  return false;
}

// 从假帧像素解码帧序号：B/G/R 分别保存低/中/高 8 位。
std::size_t decode_index(const cv::Mat& frame) {
  const cv::Vec3b& pixel = frame.at<cv::Vec3b>(0, 0);
  return static_cast<std::size_t>(pixel[0]) |
         (static_cast<std::size_t>(pixel[1]) << 8) |
         (static_cast<std::size_t>(pixel[2]) << 16);
}

// 1. start() 后能取得带递增序号的帧。
bool test_increasing_sequence() {
  auto source = std::make_unique<FakeFrameSource>();
  CameraSession session{std::move(source)};
  if (!session.start()) {
    return expect("start", false);
  }

  bool ok = true;
  for (std::size_t i = 0; i < 5; ++i) {
    cv::Mat frame;
    if (!session.wait_for_frame(frame, 1s)) {
      ok = expect("frame " + std::to_string(i) + " delivered", false);
      break;
    }
    ok = expect("frame " + std::to_string(i) + " sequence",
                decode_index(frame) == i);
    if (!ok) {
      break;
    }
  }
  session.stop();
  return ok;
}

// 2. 重复 start() 的行为符合契约：运行中拒绝；停止后可重启。
bool test_repeated_start() {
  auto source = std::make_unique<FakeFrameSource>();
  CameraSession session{std::move(source)};

  const bool first = session.start();
  const bool second = session.start();  // 运行中重复 start 应返回 false

  cv::Mat frame;
  const bool got = session.wait_for_frame(frame, 1s);
  session.stop();

  const bool restarted = session.start();  // 停止后可再次启动
  session.stop();

  return expect("first start", first) &&
         expect("second start rejected", !second) &&
         expect("frame while running", got) &&
         expect("restart allowed after stop", restarted);
}

// 3. 连续调用 stop() 不会崩溃或死锁。
bool test_repeated_stop() {
  auto source = std::make_unique<FakeFrameSource>();
  CameraSession session{std::move(source)};

  session.start();
  session.stop();
  session.stop();
  session.stop();

  cv::Mat frame;
  const bool got = session.wait_for_frame(frame, 50ms);
  return expect("stop is idempotent", !got);
}

// 4. 对象析构前工作线程已经被 join()（析构不崩溃、不遗留线程）。
bool test_destructor_joins() {
  auto source = std::make_unique<FakeFrameSource>();
  {
    CameraSession session{std::move(source)};
    session.start();
    cv::Mat frame;
    static_cast<void>(session.wait_for_frame(frame, 1s));
    // 离开作用域时析构：内部会 stop() 并 join() 工作线程。
  }
  return expect("destructor joined worker", true);
}

// 5. 消费较慢时缓冲区始终不超过两帧，并按约定丢弃旧帧。
bool test_bounded_buffer_and_drop() {
  auto source = std::make_unique<FakeFrameSource>();  // 无限产帧
  CameraSession session{std::move(source)};
  session.start();

  // 消费者很慢：给生产者时间把缓冲填满并持续丢帧。
  std::this_thread::sleep_for(100ms);

  std::size_t consumed = 0;
  cv::Mat frame;
  while (consumed < 3 && session.wait_for_frame(frame, 16ms)) {
    ++consumed;
  }
  session.stop();

  const std::size_t produced = session.frames_produced();
  const std::size_t dropped = session.dropped_frames();
  const std::size_t buffered = session.buffered_frames();

  const bool invariant =
      produced == dropped + consumed + buffered;
  return expect("buffer bounded by two frames",
                buffered <= CameraSession::kMaxBufferedFrames) &&
         expect("slow consumer caused drops", dropped > 0) &&
         expect("stats invariant", invariant);
}

// 6. 假设备抛出的读取错误能在主线程被观察。
bool test_background_error_observable() {
  FakeFrameSource::Options options;
  options.frames_before_failure = 3;
  options.failure_mode = FakeFrameSource::FailureMode::Throw;
  auto source = std::make_unique<FakeFrameSource>(options);
  CameraSession session{std::move(source)};
  session.start();

  std::size_t delivered = 0;
  cv::Mat frame;
  while (delivered < 3 && session.wait_for_frame(frame, 1s)) {
    ++delivered;
  }

  const bool got_fourth = session.wait_for_frame(frame, 1s);
  const bool error_seen = session.has_error();
  const bool message_ok =
      session.error_message().find("fake camera read failure") !=
      std::string::npos;
  session.stop();

  return expect("three frames delivered", delivered == 3) &&
         expect("fourth frame not delivered", !got_fourth) &&
         expect("background error observed", error_seen) &&
         expect("error message propagated", message_ok);
}

// 7. 空帧不会被发布为有效画面，并作为错误上报。
bool test_empty_frame_not_published() {
  FakeFrameSource::Options options;
  options.frames_before_failure = 2;
  options.failure_mode = FakeFrameSource::FailureMode::Empty;
  auto source = std::make_unique<FakeFrameSource>(options);
  CameraSession session{std::move(source)};
  session.start();

  std::size_t delivered = 0;
  cv::Mat frame;
  while (delivered < 2 && session.wait_for_frame(frame, 1s)) {
    ++delivered;
  }

  const bool got_third = session.wait_for_frame(frame, 1s);
  const bool error_seen = session.has_error();
  session.stop();

  return expect("two valid frames delivered", delivered == 2) &&
         expect("empty frame not published", !got_third) &&
         expect("empty frame reported as error", error_seen);
}

// 额外：read() 返回 false 的读失败同样能被观察并停止。
bool test_read_failure_observable() {
  FakeFrameSource::Options options;
  options.frames_before_failure = 1;
  options.failure_mode = FakeFrameSource::FailureMode::ReturnFalse;
  auto source = std::make_unique<FakeFrameSource>(options);
  CameraSession session{std::move(source)};
  session.start();

  cv::Mat frame;
  const bool first = session.wait_for_frame(frame, 1s);
  const bool second = session.wait_for_frame(frame, 1s);
  const bool error_seen = session.has_error();
  session.stop();

  return expect("first frame delivered", first) &&
         expect("failure stops delivery", !second) &&
         expect("failure observed", error_seen);
}

}  // namespace

int main() {
  bool passed = true;
  passed &= test_increasing_sequence();
  passed &= test_repeated_start();
  passed &= test_repeated_stop();
  passed &= test_destructor_joins();
  passed &= test_bounded_buffer_and_drop();
  passed &= test_background_error_observable();
  passed &= test_empty_frame_not_published();
  passed &= test_read_failure_observable();
  return passed ? 0 : 1;
}
