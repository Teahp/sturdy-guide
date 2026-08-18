#include "camera/camera_session.hpp"
#include "camera/fake_frame_source.hpp"
#include "camera/frame_source.hpp"

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string_view>
#include <thread>

namespace {

using namespace std::chrono_literals;
using sturdy_guide::camera::CameraSession;
using sturdy_guide::camera::CameraSessionState;
using sturdy_guide::camera::FakeFrameSource;
using sturdy_guide::camera::FakeFrameSourceConfig;
using sturdy_guide::camera::FrameSource;

bool expect(const std::string_view name, const bool condition) {
  if (condition) {
    return true;
  }
  std::cerr << name << " failed\n";
  return false;
}

template <typename Predicate>
bool wait_until(Predicate&& predicate, const std::chrono::milliseconds timeout =
                                      500ms) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    if (predicate()) {
      return true;
    }
    std::this_thread::sleep_for(1ms);
  }
  return predicate();
}

std::optional<sturdy_guide::camera::CapturedFrame> take_frame_when_available(
    CameraSession& session) {
  if (!wait_until([&session] { return session.buffered_frame_count() > 0; })) {
    return std::nullopt;
  }
  return session.try_take_latest();
}

bool test_frames_have_increasing_sequence_numbers() {
  CameraSession session{
      std::make_unique<FakeFrameSource>(FakeFrameSourceConfig{{8, 6}, 1ms})};
  session.start();

  const auto first = take_frame_when_available(session);
  const auto second = take_frame_when_available(session);
  session.stop();

  return expect("first fake frame is available", first.has_value()) &&
         expect("second fake frame is available", second.has_value()) &&
         expect("fake frame sequence increases",
                first && second && second->sequence > first->sequence);
}//测试帧序列号递增

bool test_repeated_start_is_rejected() {
  CameraSession session{
      std::make_unique<FakeFrameSource>(FakeFrameSourceConfig{{8, 6}, 1ms})};
  session.start();

  bool rejected = false;
  try {
    session.start();
  } catch (const std::logic_error&) {
    rejected = true;
  }
  session.stop();
  return expect("second start is rejected", rejected);
}//测试重复启动被拒绝

bool test_stop_is_idempotent() {
  CameraSession session{
      std::make_unique<FakeFrameSource>(FakeFrameSourceConfig{{8, 6}, 5ms})};
  session.start();
  session.stop();
  session.stop();
  session.stop();


  return expect("repeated stop leaves session stopped",
                session.state() == CameraSessionState::Stopped);
}//测试停止是幂等的

struct LifetimeProbe {
  std::mutex mutex;
  std::condition_variable changed;
  bool reading = false;
  bool stop_requested = false;
  bool destroyed_while_reading = false;
};

class BlockingFrameSource final : public FrameSource {
 public:
  explicit BlockingFrameSource(std::shared_ptr<LifetimeProbe> probe)
      : probe_(std::move(probe)) {}

  ~BlockingFrameSource() override {
    const std::scoped_lock lock{probe_->mutex};
    if (probe_->reading) {
      probe_->destroyed_while_reading = true;
    }
  }

  cv::Mat read() override {
    std::unique_lock lock{probe_->mutex};
    probe_->reading = true;
    probe_->changed.notify_all();
    probe_->changed.wait(lock, [this] { return probe_->stop_requested; });
    probe_->reading = false;
    probe_->changed.notify_all();
    return {};
  }

  void request_stop() noexcept override {
    {
      const std::scoped_lock lock{probe_->mutex};
      probe_->stop_requested = true;
    }
    probe_->changed.notify_all();
  }

 private:
  std::shared_ptr<LifetimeProbe> probe_;
};

bool test_destructor_joins_before_destroying_source() {
  const auto probe = std::make_shared<LifetimeProbe>();
  {
    CameraSession session{std::make_unique<BlockingFrameSource>(probe)};
    session.start();
    const bool read_started = wait_until([&probe] {
      const std::scoped_lock lock{probe->mutex};
      return probe->reading;
    });
    if (!expect("blocking read started", read_started)) {
      return false;
    }
  }

  const std::scoped_lock lock{probe->mutex};
  return expect("source was not destroyed while read was active",
                !probe->destroyed_while_reading);
}//

bool test_buffer_is_bounded_and_prefers_latest_frame() {
  CameraSession session{
      std::make_unique<FakeFrameSource>(FakeFrameSourceConfig{{8, 6}, 1ms})};
  session.start();

  const bool buffer_filled =
      wait_until([&session] { return session.buffered_frame_count() == 2; });
  const std::size_t buffered = session.buffered_frame_count();
  const auto latest = session.try_take_latest();
  session.stop();

  return expect("producer fills the bounded buffer", buffer_filled) &&
         expect("buffer never contains more than two frames", buffered <= 2) &&
         expect("consumer receives the newest buffered frame",
                latest && latest->sequence >= 2);
}

bool test_worker_error_reaches_main_thread() {
  FakeFrameSourceConfig config{{8, 6}, 1ms};
  config.fail_after_successes = 2;
  CameraSession session{std::make_unique<FakeFrameSource>(config)};
  session.start();

  const bool failed = wait_until([&session] {
    return session.state() == CameraSessionState::Failed;
  });
  bool propagated = false;
  try {
    session.rethrow_if_error();
  } catch (const std::runtime_error& error) {
    propagated = std::string_view{error.what()} == "simulated fake frame failure";
  }
  session.stop();

  return expect("worker records its read error", failed) &&
         expect("main thread observes worker error", propagated);
}

bool test_empty_frames_are_not_published() {
  FakeFrameSourceConfig config{{8, 6}, 25ms};
  config.empty_read_count = 1;
  auto source = std::make_unique<FakeFrameSource>(config);
  FakeFrameSource* const fake = source.get();
  CameraSession session{std::move(source)};
  session.start();

  const bool empty_read_returned =
      wait_until([fake] { return fake->empty_frames_returned() == 1; });
  const std::size_t buffered = session.buffered_frame_count();
  session.stop();

  return expect("fake source returned an empty frame", empty_read_returned) &&
         expect("empty frame was not published", buffered == 0);
}

}  // namespace

int main() {
  bool passed = true;
  passed &= test_frames_have_increasing_sequence_numbers();
  passed &= test_repeated_start_is_rejected();
  passed &= test_stop_is_idempotent();
  passed &= test_destructor_joins_before_destroying_source();
  passed &= test_buffer_is_bounded_and_prefers_latest_frame();
  passed &= test_worker_error_reaches_main_thread();
  passed &= test_empty_frames_are_not_published();
  return passed ? 0 : 1;
}
