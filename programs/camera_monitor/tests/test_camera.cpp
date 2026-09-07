#include "camera/fake_frame_source.hpp"
#include "camera/camera_session.hpp"
#include <cassert>
#include <thread>
#include <chrono>

using namespace sturdy_guide::camera;

void test_start_stop() {
    auto source = std::make_unique<FakeFrameSource>();
    CameraSession session(std::move(source));
    session.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    session.stop();
    session.stop(); // 二次停止安全
}

void test_buffer_limit() {
    auto source = std::make_unique<FakeFrameSource>();
    CameraSession session(std::move(source));
    session.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    for (int i = 0; i < 10; ++i) {
        auto frame = session.getLatestFrame();
        assert(frame.has_value());
    }
    session.stop();
}

void test_exception_propagation() {
    class BadSource : public FrameSource {
    public:
        std::optional<cv::Mat> read() override { return std::nullopt; }
        int width() const override { return 640; }
        int height() const override { return 480; }
    };
    auto source = std::make_unique<BadSource>();
    CameraSession session(std::move(source));
    session.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    bool caught = false;
    try {
        session.checkException();
    } catch (const std::runtime_error&) {
        caught = true;
    }
    assert(caught);
    session.stop();
}

int main() {
    test_start_stop();
    test_buffer_limit();
    test_exception_propagation();
    return 0;
}
