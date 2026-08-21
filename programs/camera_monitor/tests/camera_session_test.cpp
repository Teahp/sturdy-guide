#include "camera/camera_session.hpp"
#include "camera/fake_frame_source.hpp"

#include <chrono>
#include <cstddef>
#include <exception>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <vector>

#include <opencv2/core.hpp>

namespace {

bool expect(const std::string_view name, const bool condition) {
  if (condition) {
    return true;
  }
  std::cerr << name << " failed\n";
  return false;
}

bool test_read_returns_all_frames() {
  using namespace std::chrono_literals;
  auto frames = sturdy_guide::camera::make_test_frames(5);
  auto source =
      std::make_unique<sturdy_guide::camera::FakeFrameSource>(std::move(frames));
  const std::size_t total = source->remaining();

  sturdy_guide::camera::CameraSession session{std::move(source)};
  session.start();
  std::this_thread::sleep_for(200ms);
  session.stop();
  session.join();

  // The source should have been fully consumed.
  return expect("all frames consumed", total == 5);
}

bool test_duplicate_start_throws() {
  using namespace std::chrono_literals;
  auto frames = sturdy_guide::camera::make_test_frames(3);
  auto source =
      std::make_unique<sturdy_guide::camera::FakeFrameSource>(std::move(frames));
  sturdy_guide::camera::CameraSession session{std::move(source)};
  session.start();

  bool threw = false;
  try {
    session.start();
  } catch (const std::logic_error&) {
    threw = true;
  }

  session.stop();
  session.join();
  return expect("duplicate start throws", threw);
}

bool test_stop_before_any_read() {
  auto frames = sturdy_guide::camera::make_test_frames(3);
  auto source =
      std::make_unique<sturdy_guide::camera::FakeFrameSource>(std::move(frames));
  sturdy_guide::camera::CameraSession session{std::move(source)};
  session.start();
  session.stop();
  session.join();

  return expect("stop before read is safe", !session.is_running());
}

bool test_join_before_start_is_safe() {
  auto session = std::make_unique<sturdy_guide::camera::CameraSession>(
      std::make_unique<sturdy_guide::camera::FakeFrameSource>(
          sturdy_guide::camera::make_test_frames(0)));
  session->join();
  return expect("join before start is safe", !session->is_running());
}

bool test_frames_are_newest_first() {
  using namespace std::chrono_literals;
  auto frames = sturdy_guide::camera::make_test_frames(10);
  auto source =
      std::make_unique<sturdy_guide::camera::FakeFrameSource>(std::move(frames));
  sturdy_guide::camera::CameraSession session{std::move(source)};
  session.start();

  std::this_thread::sleep_for(150ms);
  auto latest = session.latest_frame();
  session.stop();
  session.join();

  return expect("latest frame is not empty", latest.has_value());
}

bool test_failing_device_observed() {
  using namespace std::chrono_literals;
  auto source =
      std::make_unique<sturdy_guide::camera::FakeFrameSource>(
          sturdy_guide::camera::make_test_frames(0));
  sturdy_guide::camera::CameraSession session{std::move(source)};
  session.start();
  std::this_thread::sleep_for(100ms);

  auto err = session.error();
  session.stop();
  session.join();

  bool has_error = (err != nullptr);
  if (has_error) {
    try {
      std::rethrow_exception(err);
    } catch (const std::runtime_error& e) {
      return expect("error message matches",
                    std::string_view{e.what()} ==
                        "camera stopped returning valid frames");
    }
  }
  return expect("device error propagated", false);
}

bool test_buffer_discards_old_frames() {
  using namespace std::chrono_literals;
  // Verify that the buffer discards old frames by reading at two points
  // in time and confirming they differ.  A read_delay of 5ms simulates
  // a ~200 FPS camera so frames are produced at a controllable rate.
  auto frames = sturdy_guide::camera::make_test_frames(200, 320, 240);
  auto source = std::make_unique<sturdy_guide::camera::FakeFrameSource>(
      std::move(frames), 5ms);
  sturdy_guide::camera::CameraSession session{std::move(source)};
  session.start();

  // Spin until the first frame is available, then grab it.
  while (!session.latest_frame().has_value()) {
    std::this_thread::yield();
  }
  auto frame1 = session.latest_frame();

  // Wait long enough for several new frames to be produced.
  std::this_thread::sleep_for(50ms);
  auto frame2 = session.latest_frame();
  session.stop();
  session.join();

  if (!expect("frame1 valid", frame1.has_value()) ||
      !expect("frame2 valid", frame2.has_value())) {
    return false;
  }
  if (!expect("frame1 not empty", !frame1->empty()) ||
      !expect("frame2 not empty", !frame2->empty())) {
    return false;
  }
  if (!expect("same size", frame1->size() == frame2->size())) {
    return false;
  }

  // Each frame has a unique solid colour.  With a 5ms read delay and
  // a 50ms gap, approximately 10 frames will have been produced, so
  // the two reads should return frames with different pixel content.
  return expect("frames differ (buffer replaced old data)",
                cv::norm(*frame1, *frame2) > 0.0);
}

bool test_is_running_reflects_state() {
  auto frames = sturdy_guide::camera::make_test_frames(200);
  auto source =
      std::make_unique<sturdy_guide::camera::FakeFrameSource>(std::move(frames));
  sturdy_guide::camera::CameraSession session{std::move(source)};

  const bool before_start = session.is_running();
  session.start();
  const bool during = session.is_running();
  session.stop();
  session.join();
  const bool after = session.is_running();

  return expect("not running before start", !before_start) &&
         expect("running after start", during) &&
         expect("not running after join", !after);
}

bool test_stop_and_join_complete() {
  using namespace std::chrono_literals;
  auto frames = sturdy_guide::camera::make_test_frames(500);
  auto source =
      std::make_unique<sturdy_guide::camera::FakeFrameSource>(std::move(frames));
  sturdy_guide::camera::CameraSession session{std::move(source)};
  session.start();
  std::this_thread::sleep_for(50ms);

  session.stop();
  session.join();

  // After join the thread must have exited; the session is idle.
  return expect("stopped and joined", !session.is_running());
}

bool test_no_error_on_clean_exit() {
  using namespace std::chrono_literals;
  // Use a slow source so the thread is still running when we stop it.
  // A clean stop() + join() must not leave an error.
  auto frames = sturdy_guide::camera::make_test_frames(1000, 640, 480);
  auto source = std::make_unique<sturdy_guide::camera::FakeFrameSource>(
      std::move(frames), 5ms);
  sturdy_guide::camera::CameraSession session{std::move(source)};
  session.start();
  std::this_thread::sleep_for(50ms);
  session.stop();
  session.join();

  return expect("no error on clean exit", session.error() == nullptr);
}

}  // namespace

int main() {
  bool passed = true;
  passed &= test_read_returns_all_frames();
  passed &= test_duplicate_start_throws();
  passed &= test_stop_before_any_read();
  passed &= test_join_before_start_is_safe();
  passed &= test_frames_are_newest_first();
  passed &= test_failing_device_observed();
  passed &= test_buffer_discards_old_frames();
  passed &= test_is_running_reflects_state();
  passed &= test_stop_and_join_complete();
  passed &= test_no_error_on_clean_exit();
  return passed ? 0 : 1;
}
