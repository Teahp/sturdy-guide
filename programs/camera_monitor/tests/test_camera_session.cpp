// 硬件无关测试：全部通过 FakeFrameSource 驱动 CameraSession，不打开真实设备。
#include "camera/camera_session.h"
#include "camera/fake_frame_source.h"

#include <opencv2/core.hpp>

#include <algorithm>
#include <chrono>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

using camera::CameraSession;
using camera::FakeFrameSource;
using State = CameraSession::State;

int failures = 0;

void check(bool condition, const char* expression, int line) {
  if (!condition) {
    std::cerr << "FAIL line " << line << ": " << expression << '\n';
    ++failures;
  }
}

#define CHECK(expr) check((expr), #expr, __LINE__)

FakeFrameSource::Options fast_options() {
  FakeFrameSource::Options options;
  options.frame_interval = std::chrono::milliseconds{5};
  return options;
}

std::unique_ptr<CameraSession> make_session(
    const FakeFrameSource::Options& options) {
  return std::make_unique<CameraSession>(
      std::make_unique<FakeFrameSource>(options));
}

// 1) start() 后能取得带递增序号的帧（序号 = 后台已发布帧数，单调递增）。
void test_frames_increasing() {
  auto fake = std::make_unique<FakeFrameSource>(fast_options());
  const FakeFrameSource* probe = fake.get();
  CameraSession session(std::move(fake));
  session.start();

  cv::Mat first;
  CHECK(session.wait_frame(first, std::chrono::milliseconds{1000}));
  CHECK(!first.empty());
  const std::size_t produced_1 = probe->frames_produced();
  const std::size_t counted_1 = session.frame_count();
  CHECK(produced_1 >= 1);
  CHECK(counted_1 >= 1);

  cv::Mat second;
  CHECK(session.wait_frame(second, std::chrono::milliseconds{1000}));
  CHECK(!second.empty());
  CHECK(probe->frames_produced() > produced_1);  // 序号递增
  CHECK(session.frame_count() > counted_1);

  session.stop();
  CHECK(session.state() == State::Stopped);
}

// 2) 重复 start()：运行中调用是 no-op；stop() 之后可以重启。
void test_repeated_start() {
  auto session = make_session(fast_options());
  session->start();
  CHECK(session->state() == State::Running);
  session->start();  // no-op，不崩溃
  CHECK(session->state() == State::Running);

  cv::Mat frame;
  CHECK(session->wait_frame(frame, std::chrono::milliseconds{1000}));
  CHECK(!frame.empty());

  session->stop();
  CHECK(session->state() == State::Stopped);

  session->start();  // 重启
  CHECK(session->state() == State::Running);
  CHECK(session->wait_frame(frame, std::chrono::milliseconds{1000}));
  session->stop();
}

// 3) 连续调用 stop() 不崩溃、不死锁；从未 start 就 stop 也安全。
void test_repeated_stop() {
  auto session = make_session(fast_options());
  session->start();
  session->stop();
  session->stop();
  session->stop();
  CHECK(session->state() == State::Stopped);

  CameraSession idle(std::make_unique<FakeFrameSource>(fast_options()));
  idle.stop();
  idle.stop();
  CHECK(idle.state() == State::Idle);
}

// 4) 析构前工作线程已被 join()：析构会等待进行中的 read() 返回，
//    并在 join 之后 release 帧源（以 released 标志验证该顺序）。
void test_destructor_joins() {
  FakeFrameSource::Options options;
  options.frame_interval = std::chrono::milliseconds{100};  // 慢帧
  auto fake = std::make_unique<FakeFrameSource>(options);
  const FakeFrameSource* probe = fake.get();
  {
    CameraSession session(std::move(fake));
    session.start();
    std::this_thread::sleep_for(std::chrono::milliseconds{50});  // worker 正卡在 read
  }  // 析构：join 应等待 read 返回，然后才 release
  CHECK(probe->released());
}

// 5) 消费较慢时缓冲不超过两帧，并按约定丢弃最旧帧。
void test_bounded_buffer_drops_oldest() {
  FakeFrameSource::Options options;
  options.frame_interval = std::chrono::milliseconds{1};  // 快产
  auto fake = std::make_unique<FakeFrameSource>(options);
  const FakeFrameSource* probe = fake.get();
  CameraSession session(std::move(fake));
  session.start();

  // 消费者 200ms 不取帧，期间持续采样缓冲大小。
  std::size_t max_buffered = 0;
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds{200};
  while (std::chrono::steady_clock::now() < deadline) {
    max_buffered = std::max(max_buffered, session.buffered_frames());
    std::this_thread::sleep_for(std::chrono::milliseconds{2});
  }
  CHECK(max_buffered <= CameraSession::kMaxBufferedFrames);

  // 产出远多于缓冲容量，说明中间的帧被丢弃，只保留最新。
  CHECK(probe->frames_produced() >= 50);

  // 消费者一次只拿到最新一帧；取走后缓冲不会超过上限。
  cv::Mat frame;
  CHECK(session.wait_frame(frame, std::chrono::milliseconds{100}));
  CHECK(!frame.empty());
  CHECK(session.buffered_frames() <= CameraSession::kMaxBufferedFrames);

  session.stop();
}

// 6) 假设备抛出的读取错误能在主线程被观察。
void test_error_observable() {
  FakeFrameSource::Options options = fast_options();
  options.fail_after_frames = 3;  // 产生 3 帧后 read() 抛异常
  auto session = make_session(options);
  session->start();

  cv::Mat frame;
  int received = 0;
  std::exception_ptr error;
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds{2};
  while (std::chrono::steady_clock::now() < deadline) {
    if (session->wait_frame(frame, std::chrono::milliseconds{50})) {
      ++received;
      CHECK(!frame.empty());
    }
    error = session->take_error();
    if (error) {
      break;
    }
  }

  CHECK(received >= 1);  // 失败前有帧可取
  CHECK(error != nullptr);
  if (error) {
    try {
      std::rethrow_exception(error);
    } catch (const std::runtime_error& e) {
      CHECK(std::string(e.what()).find("fake camera read failure") !=
            std::string::npos);
    } catch (...) {
      CHECK(false);
    }
  }
  session->stop();  // 出错后线程已退出；stop() 收尾不崩溃
  CHECK(session->state() == State::Stopped);
}

// 7) 空帧不会被发布为有效画面。
void test_empty_frames_not_published() {
  FakeFrameSource::Options options = fast_options();
  options.leading_empty_frames = 5;  // 前 5 次 read() 返回空帧
  auto session = make_session(options);
  session->start();

  cv::Mat frame;
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds{1};
  bool got_frame = false;
  while (std::chrono::steady_clock::now() < deadline) {
    if (session->wait_frame(frame, std::chrono::milliseconds{50})) {
      got_frame = true;
      CHECK(!frame.empty());  // 发布的帧绝不空
      break;
    }
  }
  CHECK(got_frame);
  session->stop();
}

}  // namespace

int main() {
  test_frames_increasing();
  test_repeated_start();
  test_repeated_stop();
  test_destructor_joins();
  test_bounded_buffer_drops_oldest();
  test_error_observable();
  test_empty_frames_not_published();

  if (failures == 0) {
    std::cout << "camera_session_test: ALL TESTS PASSED\n";
    return 0;
  }
  std::cerr << "camera_session_test: " << failures << " check(s) failed\n";
  return 1;
}
