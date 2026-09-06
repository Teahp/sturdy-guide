#include "camera/camera_session.hpp"
#include "camera/frame_source.hpp"

#include <opencv2/core.hpp>

#include <chrono>
#include <cstddef>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <utility>

namespace {

using namespace std::chrono_literals;

using sturdy_guide::camera::CameraSession;
using sturdy_guide::camera::Frame;

// 假插头：测试用，不依赖真实摄像头。
// good_reads>0 时，产出 good_reads 帧后下一次 read() 抛异常；否则一直产帧。
// emit_empty 为真时，每次 read() 都返回空帧。
class FakeFrameSource final : public sturdy_guide::camera::FrameSource {
 public:
  explicit FakeFrameSource(std::size_t good_reads = 0, bool emit_empty = false)
      : good_reads_(good_reads), emit_empty_(emit_empty) {}

  cv::Mat read() override {
    ++reads_;
    if (emit_empty_) {
      return cv::Mat();  // 空帧，模拟"读到了空画面"。
    }
    if (good_reads_ != 0 && reads_ > good_reads_) {
      throw std::runtime_error("fake camera failure");
    }
    // 生产速度别太快，避免测试期间忙转。
    std::this_thread::sleep_for(1ms);
    return cv::Mat(16, 24, CV_8UC3, cv::Scalar(0, 200, 80));
  }

  [[nodiscard]] std::size_t read_count() const { return reads_; }

 private:
  std::size_t reads_ = 0;
  std::size_t good_reads_;
  bool emit_empty_;
};

bool expect(const std::string_view name, const bool condition) {
  if (condition) {
    return true;
  }
  std::cerr << name << " failed\n";
  return false;
}

bool wait_for_frame(CameraSession& session, Frame& out,
                    const int iterations = 2000) {
  for (int i = 0; i < iterations; ++i) {
    if (session.take_latest(out)) {
      return true;
    }
    std::this_thread::sleep_for(1ms);
  }
  return false;
}

bool wait_for_error(CameraSession& session, std::string& message,
                    const int iterations = 2000) {
  for (int i = 0; i < iterations; ++i) {
    if (const std::exception_ptr error = session.take_error()) {
      try {
        std::rethrow_exception(error);
      } catch (const std::exception& reported) {
        message = reported.what();
      }
      return true;
    }
    std::this_thread::sleep_for(1ms);
  }
  return false;
}

// 1) start() 后能取到带递增序号的帧。
bool test_increasing_sequence() {
  CameraSession session{std::make_unique<FakeFrameSource>()};
  session.start();

  bool increasing = true;
  std::size_t previous = 0;
  bool first = true;
  std::size_t collected = 0;
  for (int i = 0; i < 2000 && collected < 6; ++i) {
    Frame frame;
    if (session.take_latest(frame)) {
      if (!first && frame.sequence <= previous) {
        increasing = false;
      }
      previous = frame.sequence;
      first = false;
      ++collected;
    } else {
      std::this_thread::sleep_for(1ms);
    }
  }
  session.stop();
  return expect("frames collected", collected >= 2) &&
         expect("sequence strictly increasing", increasing);
}

// 2) 重复 start() 抛 std::logic_error（写明的契约）。
bool test_double_start_throws() {
  CameraSession session{std::make_unique<FakeFrameSource>()};
  session.start();
  bool rejected = false;
  try {
    session.start();
  } catch (const std::logic_error&) {
    rejected = true;
  }
  session.stop();
  return expect("second start rejected", rejected);
}

// 3) 连续调用 stop() 不会崩溃或死锁（含从未 start 的情况）。
bool test_repeated_stop_is_safe() {
  bool ran_to_completion = true;
  {
    CameraSession never_started{std::make_unique<FakeFrameSource>()};
    never_started.stop();
    never_started.stop();
  }
  {
    CameraSession started{std::make_unique<FakeFrameSource>()};
    started.start();
    std::this_thread::sleep_for(20ms);
    started.stop();
    started.stop();
  }
  return expect("repeated stop safe", ran_to_completion);
}

// 4) 对象析构前工作线程已被 join：销毁过程不崩溃、不悬挂。
bool test_destructor_joins_worker() {
  bool ran_to_completion = true;
  {
    CameraSession session{std::make_unique<FakeFrameSource>()};
    session.start();
    std::this_thread::sleep_for(30ms);
  }  // 这里自动调用析构 -> stop() + join()。
  return expect("destructor joins cleanly", ran_to_completion);
}

// 5) 消费较慢时缓冲始终不超过两帧。
bool test_buffer_is_bounded() {
  CameraSession session{std::make_unique<FakeFrameSource>()};
  session.start();

  bool bounded = true;
  std::this_thread::sleep_for(10ms);  // 让帮手先产出一些帧。
  for (int i = 0; i < 30; ++i) {
    if (session.buffered_frames() > 2) {
      bounded = false;
    }
    std::this_thread::sleep_for(1ms);
  }

  // 帮手一直在产帧而我们从未消费，缓冲应顶到上限附近；最新帧可被取走。
  Frame frame;
  const bool got_newest = session.take_latest(frame);
  session.stop();
  return expect("buffer never exceeds two", bounded) &&
         expect("newest frame still available", got_newest);
}

// 6) 假设备抛出的读取错误能在主线程被观察到。
bool test_background_error_is_observed() {
  // 产出 3 帧正常画面，之后第 4 次 read() 抛错。
  CameraSession session{
      std::make_unique<FakeFrameSource>(/*good_reads=*/3)};
  session.start();

  std::string message;
  const bool observed = wait_for_error(session, message);
  session.stop();
  return expect("background error observed", observed) &&
         expect("error message preserved",
                message.find("fake camera failure") != std::string::npos);
}

// 7) 空帧不会被发布为有效画面。
bool test_empty_frame_is_not_published() {
  CameraSession session{std::make_unique<FakeFrameSource>(0, /*emit_empty=*/true)};
  session.start();

  std::string message;
  const bool error_observed = wait_for_error(session, message);
  Frame frame;
  const bool nothing_published = !session.take_latest(frame);
  session.stop();
  return expect("empty frame triggers error", error_observed) &&
         expect("empty frame not published", nothing_published);
}

}  // namespace

int main() {
  bool passed = true;
  passed &= test_increasing_sequence();
  passed &= test_double_start_throws();
  passed &= test_repeated_stop_is_safe();
  passed &= test_destructor_joins_worker();
  passed &= test_buffer_is_bounded();
  passed &= test_background_error_is_observed();
  passed &= test_empty_frame_is_not_published();
  return passed ? 0 : 1;
}
