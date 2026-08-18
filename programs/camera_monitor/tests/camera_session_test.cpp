#include "camera_monitor/camera_session.hpp"
#include "camera_monitor/fake_frame_source.hpp"

#include <cassert>
#include <chrono>
#include <opencv2/core.hpp>
#include <stdexcept>
#include <thread>

namespace {

cv::Mat frame_with_value(unsigned char value) {
    return cv::Mat(4, 4, CV_8UC3, cv::Scalar(value, value, value));
}

void test_reads_frame_from_fake_source() {
    camera_monitor::FakeFrameSource source;
    camera_monitor::CameraSession session(source);

    session.start();
    source.push(frame_with_value(7));

    cv::Mat frame;
    assert(session.wait_pop(frame));
    assert(!frame.empty());
    assert(frame.at<cv::Vec3b>(0, 0)[0] == 7);

    source.close();
    session.stop();
    session.join();
}

void test_empty_frame_is_ignored() {
    camera_monitor::FakeFrameSource source;
    camera_monitor::CameraSession session(source);

    session.start();
    source.push_empty();
    source.push(frame_with_value(3));

    cv::Mat frame;
    assert(session.wait_pop(frame));
    assert(!frame.empty());
    assert(frame.at<cv::Vec3b>(0, 0)[0] == 3);

    source.close();
    session.stop();
    session.join();
}

void test_buffer_keeps_at_most_two_frames() {
    camera_monitor::FakeFrameSource source;
    camera_monitor::CameraSession session(source);

    session.start();
    for (int i = 0; i < 8; ++i) {
        source.push(frame_with_value(static_cast<unsigned char>(i)));
    }

    cv::Mat frame;
    assert(session.wait_pop(frame));

    for (int i = 0; i < 200 && session.buffered_count() < 2; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    assert(session.buffered_count() <= 2);

    source.close();
    session.stop();
    session.join();
}

void test_worker_exception_is_rethrown_by_join() {
    camera_monitor::FakeFrameSource source;
    camera_monitor::CameraSession session(source);

    session.start();
    source.fail_next_read();

    bool threw = false;
    try {
        session.join();
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);
}

void test_stop_is_idempotent() {
    camera_monitor::FakeFrameSource source;
    camera_monitor::CameraSession session(source);

    session.start();
    source.close();
    session.stop();
    session.stop();
    session.join();
    assert(!session.is_running());
}

}  // namespace

int main() {
    test_reads_frame_from_fake_source();
    test_empty_frame_is_ignored();
    test_buffer_keeps_at_most_two_frames();
    test_worker_exception_is_rethrown_by_join();
    test_stop_is_idempotent();
    return 0;
}
