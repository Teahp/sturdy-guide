// 硬件无关测试：所有用例都通过 FakeFrameSource 驱动 CameraSession，
// 绝不打开真实设备编号，也不需要 OpenCV 窗口环境。
#include "camera/camera_session.hpp"
#include "camera/fake_frame_source.hpp"

#include <chrono>
#include <cstddef>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>

namespace {

using camera::CameraSession;
using camera::FakeFrameSource;

bool expect(const std::string_view name, const bool condition) {
  if (condition) {
    return true;
  }
  std::cerr << name << " failed\n";
  return false;
}

template <typename Predicate, typename Duration>
bool waitUntil(Predicate predicate, const Duration timeout) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (!predicate()) {
    if (std::chrono::steady_clock::now() >= deadline) {
      return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
  }
  return true;
}

// 1) start() 后能取得带递增序号的帧。
bool test_increasing_sequence_numbers() {
  bool ok = true;
  FakeFrameSource::Options options;
  options.read_delay = std::chrono::milliseconds{5};
  auto session =
      CameraSession(std::make_unique<FakeFrameSource>(options));

  ok &= expect("seq: not running before start", !session.running());
  session.start();
  ok &= expect("seq: running after start", session.running());

  std::size_t previous = 0;
  for (int i = 0; i < 5; ++i) {
    cv::Mat frame;
    std::size_t sequence = 0;
    if (!session.waitForFrame(frame, std::chrono::milliseconds{500},
                              &sequence)) {
      ok &= expect("seq: frame available", false);
      break;
    }
    ok &= expect("seq: frame is non-empty", !frame.empty());
    ok &= expect("seq: strictly increasing", sequence > previous);
    previous = sequence;
  }
  ok &= expect("seq: first sequence is one", previous >= 1);

  session.stop();
  ok &= expect("seq: not running after stop", !session.running());
  return ok;
}

// 2) 重复 start() 的行为符合文档契约：
//    已运行时是幂等 no-op（不会再次 open、不会创建第二个线程）；
//    stop() 之后可以重新 start()（重新 open，统计清零）。
bool test_repeated_start_contract() {
  bool ok = true;
  FakeFrameSource::Options options;
  options.read_delay = std::chrono::milliseconds{5};  // 让帧按可控速率到达
  auto source = std::make_unique<FakeFrameSource>(options);
  auto* raw = source.get();
  auto session = CameraSession(std::move(source));

  session.start();
  session.start();  // 已运行：no-op
  ok &= expect("start-twice: open called once", raw->open_calls() == 1);
  ok &= expect("start-twice: still running", session.running());

  cv::Mat frame;
  std::size_t sequence = 0;
  ok &= expect("start-twice: frame flows",
               session.waitForFrame(frame, std::chrono::milliseconds{500},
                                    &sequence));
  ok &= expect("start-twice: first sequence is one", sequence == 1);

  session.stop();
  session.start();  // 重启
  ok &= expect("start-again: open called again", raw->open_calls() == 2);
  ok &= expect("start-again: stats reset",
               session.capturedFrames() == 0);
  ok &= expect("start-again: frame flows again",
               session.waitForFrame(frame, std::chrono::milliseconds{500},
                                    &sequence));
  ok &= expect("start-again: renumbered from one", sequence == 1);
  session.stop();
  return ok;
}

// 3) 连续调用 stop() 不会崩溃或死锁；未启动时 stop() 也是安全的。
bool test_repeated_stop() {
  bool ok = true;
  auto session = CameraSession(std::make_unique<FakeFrameSource>());

  session.stop();  // 从未启动
  session.stop();
  session.start();
  session.stop();
  session.stop();
  session.stop();
  ok &= expect("stop-repeat: not running", !session.running());
  ok &= expect("stop-repeat: no error", session.error().empty());

  // 并发 stop() 也必须安全：lifecycle mutex 保证 join 恰好执行一次。
  auto concurrent = CameraSession(std::make_unique<FakeFrameSource>());
  concurrent.start();
  std::thread first{[&] { concurrent.stop(); }};
  std::thread second{[&] { concurrent.stop(); }};
  first.join();
  second.join();
  ok &= expect("stop-concurrent: not running", !concurrent.running());
  return ok;
}

// 4) 对象析构前工作线程已经被 join()：
//    会话析构会先 stop()（join），之后才释放 FrameSource；
//    通过共享标志确认假设备是在线程停止之后才被销毁的。
bool test_destructor_joins_worker() {
  bool ok = true;
  FakeFrameSource::Options options;
  options.read_delay = std::chrono::milliseconds{20};  // 保持线程持续工作
  std::shared_ptr<bool> destroyed;
  {
    auto source = std::make_unique<FakeFrameSource>(options);
    destroyed = source->destroyedFlag();
    *destroyed = false;

    auto session = CameraSession(std::move(source));
    session.start();
    ok &= expect("dtor: worker produced at least one frame",
                 waitUntil([&] { return session.capturedFrames() >= 1; },
                           std::chrono::seconds{2}));
    session.stop();  // 显式停止并 join
    ok &= expect("dtor: stop joined the worker", !session.running());
    session.start();  // 再次启动，让析构发生时线程仍在运行
    ok &= expect("dtor: restarted", session.running());
  }  // 会话析构：stop() → join() → 之后才释放 FrameSource

  ok &= expect("dtor: source destroyed after join", *destroyed);
  return ok;
}

// 5) 消费较慢时缓冲区始终不超过两帧，并按约定丢弃最旧帧：
//    突发 5 帧后只保留最新两帧，消费方只能取到最新一帧（序号 5），
//    被丢弃的是最旧的 3 帧。
bool test_bounded_buffer_drops_oldest() {
  bool ok = true;
  FakeFrameSource::Options options;
  options.frames = 5;  // 立即突发 5 帧，之后设备停止返回有效帧
  auto session =
      CameraSession(std::make_unique<FakeFrameSource>(options));

  session.start();
  ok &= expect("buffer: all five frames captured",
               waitUntil([&] { return session.capturedFrames() >= 5; },
                         std::chrono::seconds{2}));

  // 消费方此刻才取帧：缓冲区最多保留两帧，因此只能取到最新一帧。
  cv::Mat frame;
  std::size_t sequence = 0;
  ok &= expect("buffer: newest frame available",
               session.waitForFrame(frame, std::chrono::milliseconds{100},
                                    &sequence));
  ok &= expect("buffer: got the latest frame (5)", sequence == 5);
  ok &= expect("buffer: three oldest frames dropped",
               session.droppedFrames() == 3);
  ok &= expect("buffer: produced five frames total",
               session.capturedFrames() == 5);

  // 取走最新帧后缓冲清空，不会再有残留帧。
  ok &= expect("buffer: no leftover frames",
               !session.tryTakeFrame(frame));
  session.stop();
  return ok;
}

// 6) 假设备抛出的读取错误能在主线程被观察。
bool test_background_exception_observed() {
  bool ok = true;
  FakeFrameSource::Options options;
  options.throw_after = 3;  // 前两帧正常，第三次读取抛出
  auto session =
      CameraSession(std::make_unique<FakeFrameSource>(options));

  session.start();
  ok &= expect("throw: two frames published before failure",
               waitUntil([&] { return session.capturedFrames() >= 2; },
                         std::chrono::seconds{2}));
  ok &= expect("throw: error becomes visible",
               waitUntil([&] { return !session.error().empty(); },
                         std::chrono::seconds{2}));
  ok &= expect("throw: error message preserved",
               session.error() == "fake source failed on read 3");
  ok &= expect("throw: session left running state", !session.running());

  // 失败前已发布的帧仍可正常取走，之后不会有新帧。
  cv::Mat frame;
  std::size_t sequence = 0;
  ok &= expect("throw: last valid frame still available",
               session.waitForFrame(frame, std::chrono::milliseconds{50},
                                    &sequence));
  ok &= expect("throw: it is the second frame", sequence == 2);
  ok &= expect("throw: no frame after failure",
               !session.waitForFrame(frame, std::chrono::milliseconds{50}));
  session.stop();
  return ok;
}

// 6b) read() 返回 false（设备停止返回有效帧）同样在调用线程可观察。
bool test_read_failure_observed() {
  bool ok = true;
  FakeFrameSource::Options options;
  options.fail_after = 3;
  auto session =
      CameraSession(std::make_unique<FakeFrameSource>(options));

  session.start();
  ok &= expect("fail: error becomes visible",
               waitUntil([&] { return !session.error().empty(); },
                         std::chrono::seconds{2}));
  ok &= expect("fail: message matches starter behavior",
               session.error() == "camera stopped returning valid frames");
  ok &= expect("fail: two frames were published before failure",
               session.capturedFrames() == 2);
  session.stop();
  return ok;
}

// 7) 空帧不会被发布为有效画面。
bool test_empty_frame_not_published() {
  bool ok = true;
  FakeFrameSource::Options options;
  options.empty_at = 1;  // 第一次读取返回空帧
  auto session =
      CameraSession(std::make_unique<FakeFrameSource>(options));

  session.start();
  ok &= expect("empty: error becomes visible",
               waitUntil([&] { return !session.error().empty(); },
                         std::chrono::seconds{2}));
  ok &= expect("empty: nothing was published",
               session.capturedFrames() == 0);

  cv::Mat frame;
  ok &= expect("empty: no frame ever handed out",
               !session.waitForFrame(frame, std::chrono::milliseconds{50}));
  session.stop();
  return ok;
}

// 额外：open() 失败时 start() 同步抛出，且不创建线程、保持 Idle。
bool test_open_failure_propagates() {
  bool ok = true;
  FakeFrameSource::Options options;
  options.open_throws = true;
  auto session =
      CameraSession(std::make_unique<FakeFrameSource>(options));

  bool threw = false;
  try {
    session.start();
  } catch (const std::runtime_error& error) {
    threw = true;
    ok &= expect("open-fail: message", error.what() ==
                                           std::string_view("fake camera failed to open"));
  }
  ok &= expect("open-fail: start() threw", threw);
  ok &= expect("open-fail: never ran", !session.running());
  session.stop();  // 未启动时 stop() 也是安全的
  return ok;
}

}  // namespace

int main() {
  int failures = 0;
  failures += test_increasing_sequence_numbers() ? 0 : 1;
  failures += test_repeated_start_contract() ? 0 : 1;
  failures += test_repeated_stop() ? 0 : 1;
  failures += test_destructor_joins_worker() ? 0 : 1;
  failures += test_bounded_buffer_drops_oldest() ? 0 : 1;
  failures += test_background_exception_observed() ? 0 : 1;
  failures += test_read_failure_observed() ? 0 : 1;
  failures += test_empty_frame_not_published() ? 0 : 1;
  failures += test_open_failure_propagates() ? 0 : 1;

  if (failures == 0) {
    std::cout << "all camera_session tests passed\n";
    return 0;
  }
  std::cerr << failures << " camera_session test(s) failed\n";
  return 1;
}
