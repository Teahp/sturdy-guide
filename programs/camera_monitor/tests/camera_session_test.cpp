#include "camera/camera_session.hpp"
#include "camera/fake_frame_source.hpp"

#include <opencv2/core.hpp>

#include <chrono>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

// Reports a check and keeps a running count so main() can summarize.
bool g_passed = true;
int g_checks = 0;

void expect(bool condition, const std::string& name) {
  ++g_checks;
  if (!condition) {
    g_passed = false;
    std::cerr << "FAIL: " << name << '\n';
  } else {
    std::cout << "PASS: " << name << '\n';
  }
}

// FakeFrameSource encodes the frame sequence number in a 1x1 CV_32S Mat.
int frame_sequence(const cv::Mat& frame) { return frame.at<int>(0, 0); }

// Waits until the session reports an error or `timeout` elapses.
void wait_for_error(camera::CameraSession& session,
                    std::chrono::milliseconds timeout) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (!session.has_error() && std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds{5});
  }
}

// 1. After start(), frames with increasing sequence numbers are available.
void test_sequential_frames() {
  auto session = std::make_unique<camera::CameraSession>(
      std::make_unique<camera::FakeFrameSource>());
  session->start();

  cv::Mat first;
  const auto first_status =
      session->wait_for_frame(first, std::chrono::milliseconds{500});
  expect(first_status == camera::CameraSession::FrameStatus::NewFrame,
         "first frame arrives after start()");

  // Let the worker run ahead, then the next frame must be newer than the
  // first (the fake source produces frames at a very high rate).
  std::this_thread::sleep_for(std::chrono::milliseconds{50});
  cv::Mat second;
  const auto second_status =
      session->wait_for_frame(second, std::chrono::milliseconds{500});
  session->stop();

  expect(second_status == camera::CameraSession::FrameStatus::NewFrame,
         "second frame arrives after first");
  if (first_status == camera::CameraSession::FrameStatus::NewFrame &&
      second_status == camera::CameraSession::FrameStatus::NewFrame) {
    expect(frame_sequence(second) > frame_sequence(first),
           "frame sequence numbers strictly increase");
  }
}

// 2. Repeated start() follows the documented contract (safe no-op).
void test_repeated_start_is_noop() {
  auto session = std::make_unique<camera::CameraSession>(
      std::make_unique<camera::FakeFrameSource>());
  session->start();
  session->start();  // Must not crash, must not spawn a second worker.
  session->start();

  cv::Mat frame;
  const auto status =
      session->wait_for_frame(frame, std::chrono::milliseconds{500});
  session->stop();

  expect(status == camera::CameraSession::FrameStatus::NewFrame,
         "repeated start() keeps a single working capture thread");
}

// 3. Repeated stop() calls do not crash or deadlock.
void test_repeated_stop() {
  auto session = std::make_unique<camera::CameraSession>(
      std::make_unique<camera::FakeFrameSource>());
  session->start();
  session->stop();
  session->stop();
  session->stop();
  expect(true, "repeated stop() is safe (no crash, no deadlock)");
}

// 4. The worker thread is joined before the session is destroyed.
void test_destructor_joins_worker() {
  bool survived = false;
  {
    camera::CameraSession session{
        std::make_unique<camera::FakeFrameSource>()};
    session.start();
    cv::Mat frame;
    session.wait_for_frame(frame, std::chrono::milliseconds{500});
    // Session is destroyed here; a missing join() would std::terminate.
  }
  survived = true;
  expect(survived, "destructor joins the worker thread without crash");
}

// 5. With a slow consumer, the buffer never grows past two frames and the
//    oldest frames are dropped.
void test_bounded_buffer_drops_oldest() {
  // Produce 50 frames, then fail: the worker exits with an error.
  auto source = std::make_unique<camera::FakeFrameSource>(50);
  auto session = std::make_unique<camera::CameraSession>(std::move(source));
  session->start();

  // Let the worker run ahead while we (the consumer) do nothing.
  wait_for_error(*session, std::chrono::seconds{2});

  // Buffer must hold at most the two newest frames (49 and 50).
  cv::Mat newest;
  cv::Mat second_newest;
  cv::Mat third;
  const auto status1 =
      session->wait_for_frame(newest, std::chrono::milliseconds{100});
  const auto status2 =
      session->wait_for_frame(second_newest, std::chrono::milliseconds{100});
  const auto status3 = session->wait_for_frame(third, std::chrono::milliseconds{100});
  session->stop();

  expect(status1 == camera::CameraSession::FrameStatus::NewFrame &&
             frame_sequence(newest) == 50,
         "slow consumer receives the newest frame (50)");
  expect(status2 == camera::CameraSession::FrameStatus::NewFrame &&
             frame_sequence(second_newest) == 49,
         "second buffered frame is 49 (oldest were dropped)");
  expect(status3 == camera::CameraSession::FrameStatus::Ended,
         "buffer held at most two frames; third read sees the end");
}

// 6. A read error thrown by the fake device is observable on the main thread.
void test_error_propagates_to_main_thread() {
  // Frames 1..3 succeed, read 4 throws.
  auto session = std::make_unique<camera::CameraSession>(
      std::make_unique<camera::FakeFrameSource>(3));
  session->start();

  wait_for_error(*session, std::chrono::seconds{2});
  const bool observed = session->has_error();

  std::string message;
  if (std::exception_ptr error = session->error(); error != nullptr) {
    try {
      std::rethrow_exception(error);
    } catch (const std::exception& caught) {
      message = caught.what();
    }
  }
  session->stop();

  expect(observed, "background read error is observable via has_error()");
  expect(message.find("fake camera failure") != std::string::npos,
         "exception message crosses the thread boundary intact");
}

// 7. Empty frames are never published as valid frames.
void test_empty_frame_not_published() {
  auto source = std::make_unique<camera::FakeFrameSource>();
  auto* fake = source.get();
  fake->set_empty_frame_at(2);  // 2nd read returns an empty frame.
  auto session = std::make_unique<camera::CameraSession>(std::move(source));
  session->start();

  // Consume everything the session offers for a while; none may be empty.
  bool saw_empty = false;
  int received = 0;
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds{500};
  while (std::chrono::steady_clock::now() < deadline) {
    cv::Mat frame;
    const auto status =
        session->wait_for_frame(frame, std::chrono::milliseconds{50});
    if (status != camera::CameraSession::FrameStatus::NewFrame) {
      continue;
    }
    ++received;
    if (frame.empty()) {
      saw_empty = true;
    }
  }
  session->stop();

  expect(fake->reads() >= 3, "fake source produced the empty frame");
  expect(received > 0, "valid frames were received");
  expect(!saw_empty, "empty frame was never published as a valid frame");
}

}  // namespace

int main() {
  test_sequential_frames();
  test_repeated_start_is_noop();
  test_repeated_stop();
  test_destructor_joins_worker();
  test_bounded_buffer_drops_oldest();
  test_error_propagates_to_main_thread();
  test_empty_frame_not_published();

  std::cout << (g_passed ? "ALL TESTS PASSED" : "SOME TESTS FAILED") << " ("
            << g_checks << " checks)\n";
  return g_passed ? 0 : 1;
}
