#include "camera/camera_session.hpp"
#include "camera/fake_frame_source.hpp"

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <string_view>
#include <thread>
#include <vector>

namespace {

using camera::CameraSession;
using camera::FakeFrameSource;
using namespace std::chrono_literals;

bool expect_true(const std::string_view case_name, const bool value) {
  if (value) {
    return true;
  }
  std::cerr << case_name << ": condition was false\n";
  return false;
}

template <typename T, typename U>
bool expect_equal(const std::string_view case_name, const T& actual,
                  const U& expected) {
  if (actual == expected) {
    return true;
  }
  std::cerr << case_name << ": values differ\n";
  return false;
}

std::vector<FakeFrameSource::Step> numberedFrames(const int count,
                                                  const int start = 1) {
  std::vector<FakeFrameSource::Step> steps;
  steps.reserve(static_cast<std::size_t>(count));
  for (int index = 0; index < count; ++index) {
    steps.push_back(FakeFrameSource::frame(start + index, 2ms));
  }
  return steps;
}

template <typename Predicate>
bool waitUntil(const std::chrono::milliseconds timeout, Predicate predicate) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    if (predicate()) {
      return true;
    }
    std::this_thread::sleep_for(2ms);
  }
  return predicate();
}

bool startProducesSequentialFrames() {
  auto source = std::make_unique<FakeFrameSource>(numberedFrames(4));
  CameraSession session{std::move(source)};
  session.start();

  const auto first = session.waitForFrame(200ms);
  const auto second = session.waitForFrame(200ms);
  session.stop();

  bool passed = true;
  passed &= expect_true("first frame available", first.has_value());
  passed &= expect_true("second frame available", second.has_value());
  if (first && second) {
    passed &= expect_equal("first sequence", first->sequence, std::size_t{1});
    passed &= expect_equal("second sequence", second->sequence,
                           std::size_t{2});
  }
  return passed;
}

bool repeatedStartIsIdempotentWhileRunning() {
  auto source = std::make_unique<FakeFrameSource>(numberedFrames(5));
  auto* fake = source.get();
  CameraSession session{std::move(source)};

  session.start();
  session.start();
  const auto frame = session.waitForFrame(200ms);
  session.stop();

  bool passed = true;
  passed &= expect_true("frame after repeated start", frame.has_value());
  passed &= expect_equal("single open while running", fake->openCount(),
                         std::size_t{1});
  return passed;
}

bool stopIsIdempotent() {
  auto source = std::make_unique<FakeFrameSource>(numberedFrames(10));
  CameraSession session{std::move(source)};
  session.start();
  session.stop();
  session.stop();
  session.stop();
  return expect_true("state after repeated stop",
                     session.state() == CameraSession::State::Stopped);
}

bool destructorStopsAndDestroysSource() {
  auto destroyed = std::make_shared<std::atomic_bool>(false);
  {
    auto source =
        std::make_unique<FakeFrameSource>(numberedFrames(20), destroyed);
    CameraSession session{std::move(source)};
    session.start();
    (void)session.waitForFrame(200ms);
  }
  return expect_true("source destroyed after session scope",
                     destroyed->load());
}

bool slowConsumerDropsOldFrames() {
  std::vector<FakeFrameSource::Step> steps;
  for (int value = 1; value <= 5; ++value) {
    steps.push_back(FakeFrameSource::frame(value));
  }
  steps.push_back(FakeFrameSource::failure("done"));

  CameraSession session{
      std::make_unique<FakeFrameSource>(std::move(steps))};
  session.start();

  const bool captured = waitUntil(500ms, [&] {
    return session.capturedFrames() >= 5 || session.errorMessage().has_value();
  });
  const auto latest = session.tryTakeFrame();
  session.stop();

  bool passed = true;
  passed &= expect_true("five frames captured", captured);
  passed &= expect_true("latest frame available", latest.has_value());
  if (latest) {
    passed &= expect_equal("latest sequence kept", latest->sequence,
                           std::size_t{5});
  }
  passed &= expect_equal("buffer empty after take", session.bufferedFrames(),
                         std::size_t{0});
  passed &= expect_equal("dropped oldest frames", session.droppedFrames(),
                         std::size_t{3});
  return passed;
}

bool readFailureIsReported() {
  std::vector<FakeFrameSource::Step> steps{
      FakeFrameSource::frame(1),
      FakeFrameSource::failure("sensor unplugged"),
  };
  CameraSession session{
      std::make_unique<FakeFrameSource>(std::move(steps))};
  session.start();

  const bool failed = waitUntil(500ms, [&] {
    const auto error = session.errorMessage();
    return error && *error == "sensor unplugged";
  });
  session.stop();
  return expect_true("background error visible", failed);
}

bool emptyFrameIsNotPublished() {
  std::vector<FakeFrameSource::Step> steps{
      FakeFrameSource::empty(),
      FakeFrameSource::frame(2),
  };
  CameraSession session{
      std::make_unique<FakeFrameSource>(std::move(steps))};
  session.start();

  const bool failed = waitUntil(500ms, [&] {
    return session.errorMessage().has_value();
  });
  const auto frame = session.tryTakeFrame();
  session.stop();

  bool passed = true;
  passed &= expect_true("empty frame reported as error", failed);
  passed &= expect_true("empty frame not published", !frame.has_value());
  passed &= expect_equal("no captured frames", session.capturedFrames(),
                         std::size_t{0});
  return passed;
}

bool restartAfterStopResetsStatistics() {
  auto source = std::make_unique<FakeFrameSource>(numberedFrames(3));
  auto* fake = source.get();
  CameraSession session{std::move(source)};

  session.start();
  (void)session.waitForFrame(200ms);
  session.stop();
  session.start();
  const auto frame = session.waitForFrame(200ms);
  session.stop();

  bool passed = true;
  passed &= expect_true("frame after restart", frame.has_value());
  if (frame) {
    passed &= expect_equal("sequence reset after restart", frame->sequence,
                           std::size_t{1});
  }
  passed &= expect_equal("opened twice", fake->openCount(), std::size_t{2});
  return passed;
}

bool run(const std::string_view name, bool (*test)()) {
  const bool passed = test();
  std::cout << (passed ? "[PASS] " : "[FAIL] ") << name << '\n';
  return passed;
}

}  // namespace

int main() {
  bool passed = true;
  passed &= run("start produces sequential frames",
                startProducesSequentialFrames);
  passed &= run("repeated start is idempotent",
                repeatedStartIsIdempotentWhileRunning);
  passed &= run("stop is idempotent", stopIsIdempotent);
  passed &= run("destructor stops and destroys source",
                destructorStopsAndDestroysSource);
  passed &= run("slow consumer drops old frames", slowConsumerDropsOldFrames);
  passed &= run("read failure is reported", readFailureIsReported);
  passed &= run("empty frame is not published", emptyFrameIsNotPublished);
  passed &= run("restart resets statistics", restartAfterStopResetsStatistics);
  return passed ? 0 : 1;
}
