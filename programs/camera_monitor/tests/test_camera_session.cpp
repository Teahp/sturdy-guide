#include "camera/fake_frame_source.hpp"
#include "camera/camera_session.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <stdexcept>

using namespace camera;

// 测试1：start后可获取递增序号的帧
bool test_start_and_frame_sequence() {
    auto source = std::make_unique<FakeFrameSource>(640, 480);
    CameraSession session(std::move(source));
    session.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    CameraSession::Frame frame1;
    if (!session.try_get_latest_frame(frame1)) {
        std::cerr << "  Fail: no frame received\n";
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    CameraSession::Frame frame2;
    if (!session.try_get_latest_frame(frame2)) {
        std::cerr << "  Fail: second frame missing\n";
        return false;
    }

    if (frame2.id <= frame1.id) {
        std::cerr << "  Fail: frame id not incrementing\n";
        return false;
    }

    session.stop();
    session.join();
    return true;
}

// 测试2：重复start()符合契约（安全、无副作用）
bool test_repeated_start() {
    auto source = std::make_unique<FakeFrameSource>(640, 480);
    CameraSession session(std::move(source));

    session.start();
    session.start();
    session.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    CameraSession::Frame frame;
    if (!session.try_get_latest_frame(frame)) {
        std::cerr << "  Fail: no frame after repeated start\n";
        return false;
    }

    session.stop();
    session.stop();
    session.join();
    return true;
}

// 测试3：连续调用stop()不崩溃、不死锁
bool test_repeated_stop() {
    auto source = std::make_unique<FakeFrameSource>(640, 480);
    CameraSession session(std::move(source));
    session.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    session.stop();
    session.stop();
    session.stop();

    session.join();
    return true;
}

// 测试4：析构时自动join线程，无悬挂
bool test_destructor_joins_thread() {
    auto source = std::make_unique<FakeFrameSource>(640, 480);
    {
        CameraSession session(std::move(source));
        session.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        // 离开作用域自动析构，若未join则会崩溃
    }
    return true;
}

// 测试5：消费慢时缓冲区不超过2帧，旧帧被丢弃
bool test_buffer_bounded() {
    auto* fake_source = new FakeFrameSource(640, 480);
    std::unique_ptr<FrameSource> source(fake_source);
    CameraSession session(std::move(source));

    session.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    size_t produced = fake_source->frame_count();
    if (produced < 3) {
        std::cerr << "  Warning: too few frames produced\n";
    }

    CameraSession::Frame frame;
    if (!session.try_get_latest_frame(frame)) {
        std::cerr << "  Fail: no frame received\n";
        return false;
    }

    // 最新帧ID接近生产总数 → 中间旧帧已被丢弃，缓冲区有界
    if (frame.id < produced - 2) {
        std::cerr << "  Fail: frame id " << frame.id
                  << " too old (produced " << produced << ")\n";
        return false;
    }

    session.stop();
    session.join();
    return true;
}

// 测试6：后台读取异常可被主线程观察
bool test_error_propagation() {
    auto* fake_source = new FakeFrameSource(640, 480);
    fake_source->set_fail_after(3);
    std::unique_ptr<FrameSource> source(fake_source);
    CameraSession session(std::move(source));

    session.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    if (!session.has_error()) {
        std::cerr << "  Fail: no error observed\n";
        return false;
    }

    bool threw = false;
    try {
        CameraSession::Frame frame;
        session.try_get_latest_frame(frame);
    } catch (const std::runtime_error& e) {
        threw = std::string(e.what()).find("simulated read failure") != std::string::npos;
    } catch (...) {}

    if (!threw) {
        std::cerr << "  Fail: exception not rethrown in main thread\n";
        return false;
    }

    session.stop();
    session.join();
    return true;
}

// 测试7：空帧不会被发布为有效画面
bool test_empty_frame_not_published() {
    class EmptyFrameSource : public FrameSource {
    public:
        void read(cv::Mat& frame) override {
            frame = cv::Mat(); // 返回空帧
        }
    };

    auto source = std::make_unique<EmptyFrameSource>();
    CameraSession session(std::move(source));
    session.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    if (!session.has_error()) {
        std::cerr << "  Fail: empty frame did not trigger error\n";
        return false;
    }

    session.stop();
    session.join();
    return true;
}

int main() {
    int passed = 0;
    constexpr int total = 7;

    auto run = [&](const char* name, bool (*test)()) {
        std::cout << "- " << name << " ... ";
        try {
            if (test()) {
                std::cout << "PASSED\n";
                ++passed;
            } else {
                std::cout << "FAILED\n";
            }
        } catch (const std::exception& e) {
            std::cout << "FAILED (" << e.what() << ")\n";
        }
    };

    std::cout << "Running CameraSession tests:\n";
    run("start & frame sequence", test_start_and_frame_sequence);
    run("repeated start safety", test_repeated_start);
    run("repeated stop safety", test_repeated_stop);
    run("destructor joins thread", test_destructor_joins_thread);
    run("bounded buffer & drop policy", test_buffer_bounded);
    run("error cross-thread propagation", test_error_propagation);
    run("empty frame not published", test_empty_frame_not_published);

    std::cout << "\nResult: " << passed << "/" << total << " passed\n";
    return (passed == total) ? 0 : 1;
}
