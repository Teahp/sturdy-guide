#include "camera/camera_session.hpp"
#include "camera/fake_frame_source.hpp"
#include "camera/frame_source.hpp"

#include <chrono>
#include <exception>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <thread>

namespace {

using namespace std::chrono_literals;

bool expect(const std::string_view name, const bool condition) {
  if (condition) {
    return true;
  }
  std::cerr << name << " failed\n";
  return false;
}

std::unique_ptr<FrameSource> make_fake(
    const std::size_t frames_before_failure = 0) {
  return std::make_unique<FakeFrameSource>(64, 48, frames_before_failure);
}

// 轮询取帧，最多等 timeout；避免"睡固定时间然后祈祷"的不确定写法
std::optional<Frame> wait_for_frame(CameraSession& session,
                                    const std::chrono::milliseconds timeout) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    if (auto frame = session.consume_latest()) {
      return frame;
    }
    std::this_thread::sleep_for(1ms);
  }
  return std::nullopt;
}

bool wait_until_failed(CameraSession& session,
                       const std::chrono::milliseconds timeout) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    if (session.state() == SessionState::kFailed) {
      return true;
    }
    std::this_thread::sleep_for(1ms);
  }
  return false;
}

// 要求 1：start 后能取得带递增序号的帧
bool test_start_yields_increasing_frames() {
  CameraSession session{make_fake()};
  session.start();
  const auto first = wait_for_frame(session, 2s);
  const auto second = wait_for_frame(session, 2s);
  session.stop();
  return expect("start yields a frame", first.has_value()) &&
         expect("frame has image", first && !first->image.empty()) &&
         expect("second frame exists", second.has_value()) &&
         expect("sequence increases",
                second && first && second->sequence > first->sequence);
}

// 要求 2：重复 start 按契约抛 logic_error
bool test_repeated_start_throws() {
  CameraSession session{make_fake()};
  session.start();
  bool threw = false;
  try {
    session.start();
  } catch (const std::logic_error&) {
    threw = true;
  }
  session.stop();
  return expect("second start throws logic_error", threw);
}

// 要求 3：连续 stop 不崩溃、不死锁，且最终回到 Idle
bool test_repeated_stop_is_safe() {
  CameraSession session{make_fake()};
  session.stop();  // 还没 start 就 stop，也安全
  session.start();
  session.stop();
  session.stop();
  session.stop();
  return expect("repeated stop returns idle",
                session.state() == SessionState::kIdle);
}

// 要求 4：析构前工作线程已被 join（析构能正常返回就说明 join 完成）
bool test_destructor_joins() {
  {
    CameraSession session{make_fake()};
    session.start();
    static_cast<void>(wait_for_frame(session, 2s));
    // 离开作用域 → 析构函数执行 stop + join
  }
  return expect("destructor returns without hang or crash", true);
}

// 要求 5：消费慢时丢旧帧（后台产得多，取到的永远是最新）
bool test_buffer_drops_old_frames() {
  CameraSession session{make_fake()};
  session.start();
  const auto first = wait_for_frame(session, 2s);
  std::this_thread::sleep_for(100ms);  // 故意不消费，让后台猛产
  const auto latest = session.consume_latest();
  session.stop();
  if (!first || !latest) {
    return expect("buffer drop frames exist", false);
  }
  // 最新帧序号远大于第一帧 → 中间的帧被丢弃，而不是排队
  return expect("old frames dropped",
                latest->sequence - first->sequence > 10);
}

// 要求 6：假设备抛错能在主线程被观察到
bool test_fake_error_reaches_main_thread() {
  CameraSession session{make_fake(3)};  // 3 帧后抛错
  session.start();
  static_cast<void>(wait_for_frame(session, 2s));
  const bool failed = wait_until_failed(session, 2s);
  session.stop();
  if (!expect("failure observed", failed)) {
    return false;
  }
  try {
    std::rethrow_exception(session.error());
  } catch (const std::runtime_error& error) {
    return expect("error message",
                  std::string_view{error.what()} == "fake source exhausted");
  }
  return expect("error propagated", false);
}

// 要求 7：空帧不会被发布为有效画面
class EmptyFrameSource final : public FrameSource {
 public:
  Frame read() override { return Frame{cv::Mat{}, 1}; }
};

bool test_empty_frame_not_published() {
  CameraSession session{std::make_unique<EmptyFrameSource>()};
  session.start();
  const bool failed = wait_until_failed(session, 2s);
  const auto frame = session.consume_latest();
  session.stop();
  return expect("empty frame reported as failure", failed) &&
         expect("no empty frame published", !frame.has_value()) &&
         expect("error recorded", session.error() != nullptr);
}

}  // namespace

int main() {
  bool passed = true;
  passed &= test_start_yields_increasing_frames();
  passed &= test_repeated_start_throws();
  passed &= test_repeated_stop_is_safe();
  passed &= test_destructor_joins();
  passed &= test_buffer_drops_old_frames();
  passed &= test_fake_error_reaches_main_thread();
  passed &= test_empty_frame_not_published();
  return passed ? 0 : 1;
}
