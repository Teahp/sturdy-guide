#include "camera/camera_session.h"
#include "fake_frame_source.h"
#include <cassert>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

using namespace camera;
using namespace camera::testing;

// 简单测试框架（避免引入额外依赖）
#define TEST_CASE(name) \
    void name(); \
    struct name##_runner { \
        name##_runner() { \
            std::cout << "Running " << #name << "...\n"; \
            name(); \
            std::cout << "  PASSED\n"; \
        } \
    } name##_instance; \
    void name()

// 辅助函数：等待条件成立
template <typename Func>
bool waitFor(Func&& func, int timeout_ms = 1000) {
    const auto start = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - start).count() < timeout_ms) {
        if (func()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return false;
}

// 测试1：start() 后能取得带递增序号的帧
TEST_CASE(test_start_and_get_frames) {
    auto fake = std::make_unique<FakeFrameSource>();
    CameraSession session(std::move(fake));

    session.start();

    // 获取第一帧
    Frame frame1;
    assert(waitFor([&]() { return session.tryGetLatestFrame(frame1); }));
    assert(frame1.valid);
    assert(!frame1.isEmpty());

    // 等待并获取第二帧
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    Frame frame2;
    assert(waitFor([&]() { return session.tryGetLatestFrame(frame2); }));
    assert(frame2.valid);

    // 验证序号递增
    assert(frame2.sequence > frame1.sequence);
    std::cout << "  Frame 1 sequence: " << frame1.sequence << "\n";
    std::cout << "  Frame 2 sequence: " << frame2.sequence << "\n";

    session.stop();
}

// 测试2：重复 start() 的行为
TEST_CASE(test_double_start_is_safe) {
    auto fake = std::make_unique<FakeFrameSource>();
    CameraSession session(std::move(fake));

    session.start();
    assert(session.isRunning());

    // 重复 start 应该被忽略
    session.start();
    assert(session.isRunning());

    session.stop();
    assert(!session.isRunning());
}

// 测试3：连续调用 stop() 不会崩溃或死锁
TEST_CASE(test_double_stop_is_safe) {
    auto fake = std::make_unique<FakeFrameSource>();
    CameraSession session(std::move(fake));

    session.start();
    session.stop();
    session.stop();  // 应该安全返回
    session.stop();  // 多次调用也不应该有问题

    assert(!session.isRunning());
}

// 测试4：对象析构前工作线程已经被 join()
TEST_CASE(test_destructor_joins_thread) {
    auto fake = std::make_unique<FakeFrameSource>(100);  // 很快失败
    {
        CameraSession session(std::move(fake));
        session.start();
        // 等待线程启动
        assert(waitFor([&]() { return session.isRunning(); }));
        // session 析构时应该自动 stop 和 join
    }
    // 如果析构没有 join，程序会崩溃或泄漏
    std::cout << "  Destructor joined thread successfully\n";
}

// 测试5：消费较慢时缓冲区始终不超过两帧，并按约定丢帧
TEST_CASE(test_bounded_buffer_drops_old_frames) {
    auto fake_ptr = std::make_unique<FakeFrameSource>(-1, 10);  // 每 10ms 一帧
    FakeFrameSource* fake_raw = fake_ptr.get();  // 保留原始指针用于查询
    CameraSession session(std::move(fake_ptr));

    session.start();

    // 等待一些帧被采集但不消费（模拟消费慢）
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 记录已读取的帧数
    const int frames_read = fake_raw->getReadCount();
    std::cout << "  Frames read by worker: " << frames_read << "\n";

    // 现在消费，验证只保留最新帧
    Frame frame;
    if (session.tryGetLatestFrame(frame)) {
        // 验证获取到的是最新帧（序列号接近 frames_read - 1）
        assert(frame.sequence >= static_cast<uint64_t>(frames_read - 2));
        std::cout << "  Latest frame sequence: " << frame.sequence << "\n";
    }

    // 验证缓冲区中最多只有一帧待消费（双缓冲中只有最新帧被保留）
    // 再次消费，应该没有新帧
    Frame frame2;
    assert(!session.tryGetLatestFrame(frame2));

    // 关键验证：worker 已经读取了多帧（frames_read > 2），
    // 但缓冲区中只有最新帧（旧帧被丢弃）
    assert(frames_read > 2);
    std::cout << "  Buffer holds only latest frame (old frames dropped)\n";

    session.stop();
}

// 测试6：假设备抛出的读取错误能在主线程被观察
TEST_CASE(test_worker_error_propagation) {
    auto fake = std::make_unique<FakeFrameSource>(3);  // 第 3 帧后失败
    CameraSession session(std::move(fake));

    session.start();

    // 等待错误发生
    assert(waitFor([&]() { return session.hasError(); }, 2000));

    const std::string error = session.getErrorMessage();
    assert(!error.empty());
    std::cout << "  Worker error: " << error << "\n";

    session.stop();
}

// 测试7：空帧不会被发布为有效画面
TEST_CASE(test_empty_frames_not_published) {
    auto fake = std::make_unique<FakeFrameSource>(-1, 5);  // 每 5ms 一帧
    fake->setReturnEmptyEvery(2);  // 每隔一帧返回空帧

    CameraSession session(std::move(fake));

    session.start();

    // 等待一些帧（包括空帧）被处理
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // 获取到的所有帧都应该是有效的
    int valid_frames = 0;
    Frame frame;
    while (session.tryGetLatestFrame(frame)) {
        assert(frame.valid);
        assert(!frame.isEmpty());
        assert(!frame.image.empty());
        valid_frames++;
    }

    std::cout << "  Got " << valid_frames << " valid frames (no empty frames published)\n";
    assert(valid_frames > 0);  // 至少应该有一些有效帧

    session.stop();
}

// 测试8：重复 start 后重新开始采集
TEST_CASE(test_restart_after_stop) {
    auto fake = std::make_unique<FakeFrameSource>();
    CameraSession session(std::move(fake));

    // 第一次启动
    session.start();
    assert(waitFor([&]() {
        Frame f;
        return session.tryGetLatestFrame(f);
    }));
    session.stop();

    // 第二次启动
    session.start();
    assert(session.isRunning());
    Frame frame;
    assert(waitFor([&]() { return session.tryGetLatestFrame(frame); }));
    assert(frame.valid);

    session.stop();
}

int main() {
    std::cout << "Running CameraSession tests...\n\n";
    // 测试由全局对象的构造函数自动运行
    std::cout << "\nAll tests passed!\n";
    return 0;
}
