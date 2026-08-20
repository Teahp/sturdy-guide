#include "camera/camera_session.hpp"
#include "camera/frame_source.hpp"

#include <opencv2/core.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <thread>

namespace {

using sturdy_guide::camera::CameraSession;
using sturdy_guide::camera::FrameSource;

// 用于验证析构顺序：若工作线程在 FrameSource 析构后仍调用 read()，
// 说明 CameraSession 没有在释放设备前 join 线程。
struct LifetimeGuard {
  std::atomic<bool> destroyed{false};
  std::atomic<bool> accessed_after_destroy{false};
};

// 假帧来源：产生内容编码了帧序号的确定性帧，也可模拟失败或空帧。
// 帧序号从 1 开始递增；单通道像素值等于序号（截断到 0..255）。
class FakeFrameSource final : public FrameSource {
 public:
  struct Config {
    std::size_t total_frames = 0;  // 0 表示无限
    std::size_t fail_on = 0;       // 第 n 帧抛异常（0 表示从不）
    std::size_t empty_on = 0;      // 第 n 帧返回空帧（0 表示从不）
    std::chrono::milliseconds frame_delay{0};
  };

  explicit FakeFrameSource(Config config, LifetimeGuard* guard = nullptr)
      : config_{config}, guard_{guard} {}

  ~FakeFrameSource() override {
    if (guard_ != nullptr) {
      guard_->destroyed = true;
    }
  }

  cv::Mat read() override {
    if (guard_ != nullptr && guard_->destroyed.load()) {
      guard_->accessed_after_destroy = true;
      return {};
    }
    if (config_.frame_delay.count() > 0) {
      std::this_thread::sleep_for(config_.frame_delay);
    }

    const std::size_t n = ++produced_;
    if (config_.fail_on != 0 && n == config_.fail_on) {
      throw std::runtime_error("simulated camera failure");
    }
    if (config_.empty_on != 0 && n == config_.empty_on) {
      return {};
    }
    if (config_.total_frames != 0 && n > config_.total_frames) {
      return {};
    }
    return cv::Mat{16, 16, CV_8UC1,
                   cv::Scalar{static_cast<double>(n % 256)}};
  }

  [[nodiscard]] std::size_t frames_produced() const {
    return produced_.load();
  }

 private:
  Config config_;
  LifetimeGuard* guard_;
  std::atomic<std::size_t> produced_{0};
};

bool expect(const std::string_view name, const bool condition) {
  if (condition) {
    return true;
  }
  std::cerr << name << " failed\n";
  return false;
}

// 1. start() 后能取得带递增序号的帧。
bool test_incremental_frames() {
  FakeFrameSource::Config config;
  config.total_frames = 5;
  config.frame_delay = std::chrono::milliseconds{5};

  CameraSession session{std::make_unique<FakeFrameSource>(config)};
  session.start();

  bool got_any = false;
  bool all_increasing = true;
  bool all_nonempty = true;
  std::size_t last = 0;

  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds{1};
  while (std::chrono::steady_clock::now() < deadline) {
    cv::Mat image;
    std::size_t index = 0;
    if (session.try_latest_frame(image, index)) {
      got_any = true;
      all_nonempty = all_nonempty && !image.empty();
      all_increasing = all_increasing && index > last;
      last = index;
      if (index == 5) {
        break;
      }
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds{2});
    }
  }

  session.stop();
  session.wait();

  return expect("received frames", got_any) &&
         expect("frame numbers increase", all_increasing) &&
         expect("frames are not empty", all_nonempty) &&
         expect("reached final frame", last == 5);
}

// 2. 重复 start() 抛 std::logic_error（与 README 中写明的契约一致）。
bool test_double_start_throws() {
  CameraSession session{std::make_unique<FakeFrameSource>(FakeFrameSource::Config{})};
  session.start();

  bool threw = false;
  try {
    session.start();
  } catch (const std::logic_error&) {
    threw = true;
  }

  session.stop();
  session.wait();

  return expect("second start throws logic_error", threw);
}

// 3. 连续调用 stop() 不会崩溃或死锁（包括未 start 前与重复 wait()）。
bool test_repeated_stop_is_safe() {
  CameraSession session{std::make_unique<FakeFrameSource>(FakeFrameSource::Config{})};

  session.stop();  // 未 start 就 stop，应安全。
  session.stop();

  session.start();
  session.stop();
  session.stop();
  session.stop();
  session.wait();
  session.wait();  // 重复 wait 也应安全。

  return expect("repeated stop/wait is safe", true);
}

// 4. 对象析构前工作线程已被 join()。
bool test_destructor_joins_worker() {
  LifetimeGuard guard;
  {
    FakeFrameSource::Config config;
    config.frame_delay = std::chrono::milliseconds{1};
    CameraSession session{
        std::make_unique<FakeFrameSource>(config, &guard)};
    session.start();
    std::this_thread::sleep_for(std::chrono::milliseconds{10});
    // 离开作用域：析构函数必须先 stop + join，再释放 FrameSource。
  }
  return expect("worker joined before source destruction",
                !guard.accessed_after_destroy.load());
}

// 5. 消费较慢时缓冲不超过两帧，并按约定丢弃旧帧（保留最新）。
bool test_bounded_buffer_drops_oldest() {
  FakeFrameSource::Config config;
  config.frame_delay = std::chrono::milliseconds{1};

  auto* fake = new FakeFrameSource(config);
  CameraSession session{std::unique_ptr<FrameSource>(fake)};
  session.start();

  // 生产者在此期间产生远超两帧的画面，缓冲必须只保留最新。
  std::this_thread::sleep_for(std::chrono::milliseconds{100});

  cv::Mat image;
  std::size_t index = 0;
  const bool got = session.try_latest_frame(image, index);
  const std::size_t produced = fake->frames_produced();

  session.stop();
  session.wait();

  return expect("latest frame available", got) &&
         expect("frame not empty", !image.empty()) &&
         expect("many frames produced", produced > 2) &&
         expect("dropped older frames, kept recent index", index > 1) &&
         expect("index does not exceed produced", index <= produced);
}

// 6. 假设备抛出的读取错误能在主线程被观察。
bool test_read_error_crosses_thread_boundary() {
  FakeFrameSource::Config config;
  config.fail_on = 3;

  CameraSession session{std::make_unique<FakeFrameSource>(config)};
  session.start();

  try {
    session.wait();
  } catch (const std::runtime_error& error) {
    return expect("error message preserved",
                  std::string_view{error.what()} == "simulated camera failure");
  }
  return expect("read error propagated", false);
}

// 7. 空帧不会被发布为有效画面。
bool test_empty_frame_is_not_published() {
  FakeFrameSource::Config config;
  config.empty_on = 2;  // 第 1 帧有效，第 2 帧为空。

  CameraSession session{std::make_unique<FakeFrameSource>(config)};
  session.start();
  session.wait();  // 空帧导致正常停止，不抛异常。

  cv::Mat image;
  std::size_t index = 0;
  const bool got_first = session.try_latest_frame(image, index);
  const bool got_second = session.try_latest_frame(image, index);

  return expect("valid frame published", got_first && index == 1) &&
         expect("valid frame not empty", !image.empty()) &&
         expect("empty frame not published", !got_second) &&
         expect("session stopped after empty frame", !session.is_running());
}

}  // namespace

int main() {
  bool passed = true;
  passed &= test_incremental_frames();
  passed &= test_double_start_throws();
  passed &= test_repeated_stop_is_safe();
  passed &= test_destructor_joins_worker();
  passed &= test_bounded_buffer_drops_oldest();
  passed &= test_read_error_crosses_thread_boundary();
  passed &= test_empty_frame_is_not_published();
  return passed ? 0 : 1;
}
