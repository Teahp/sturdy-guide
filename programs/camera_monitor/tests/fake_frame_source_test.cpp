// FakeFrameSource 自身的契约测试。
//
// 假设备是所有并发用例的地基：如果它自己会"多读一帧""关闭后还能读"，
// 上层用例失败时会很难判断到底是实现错了还是测试替身错了。

#include "camera/fake_frame_source.hpp"

#include <cstddef>
#include <exception>
#include <iostream>
#include <memory>
#include <string>

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

void test_read_requires_open() {
  camera::FakeFrameSource source;
  cv::Mat frame;

  expect_true("is_open() is false before open()", !source.is_open(), "");
  expect_true("read() before open() reports failure", !source.read(frame), "");

  expect_true("open() succeeds", source.open(), "");
  expect_true("is_open() is true after open()", source.is_open(), "");
  expect_true("read() succeeds after open()", source.read(frame), "");
  expect_true("read() fills a non-empty image", !frame.empty(), "");
}

void test_close_is_idempotent() {
  auto probe = std::make_shared<camera::SourceProbe>();
  camera::FakeFrameSource source{{}, probe};

  (void)source.open();
  source.close();
  source.close();

  expect_true("close() can be called repeatedly",
              probe->close_calls.load() == 2,
              std::to_string(probe->close_calls.load()));
  expect_true("is_open() is false after close()", !source.is_open(), "");

  cv::Mat frame;
  expect_true("read() after close() reports failure", !source.read(frame), "");
}

void test_total_frames_exhausts() {
  camera::FakeFrameSource::Config config;
  config.total_frames = 2;
  camera::FakeFrameSource source{config};

  (void)source.open();
  cv::Mat first;
  cv::Mat second;
  cv::Mat third;

  expect_true("first read succeeds", source.read(first), "");
  expect_true("second read succeeds", source.read(second), "");
  expect_true("read past total_frames reports failure",
              !source.read(third), "");
}

void test_empty_and_throwing_reads() {
  camera::FakeFrameSource::Config config;
  config.empty_at = 1;
  config.throw_at = 2;
  camera::FakeFrameSource source{config};

  (void)source.open();
  cv::Mat frame;

  bool empty_reported = false;
  try {
    empty_reported = source.read(frame) && frame.empty();
  } catch (const std::exception&) {
    empty_reported = false;
  }
  expect_true("empty_at produces a successful read with an empty image",
              empty_reported, "");

  std::string message;
  try {
    source.read(frame);
  } catch (const std::exception& exception) {
    message = exception.what();
  }
  expect_true("throw_at raises the documented exception",
              message.find("simulated read failure") != std::string::npos,
              message);
}

}  // namespace

int main() {
  test_read_requires_open();
  test_close_is_idempotent();
  test_total_frames_exhausts();
  test_empty_and_throwing_reads();

  if (failures == 0) {
    std::cout << "fake_frame_source_test: all cases passed\n";
    return 0;
  }
  std::cerr << "fake_frame_source_test: " << failures << " case(s) failed\n";
  return 1;
}
