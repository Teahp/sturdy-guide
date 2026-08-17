#include <gtest/gtest.h>
#include "camera/session.hpp"
#include "camera/fake_frame_source.hpp"
#include <chrono>
#include <thread>

using namespace camera;

TEST(CameraSessionTest, StartAndGetFrame) {
    auto source = std::make_unique<FakeFrameSource>();
    CameraSession session(std::move(source));
    session.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    Frame frame;
    bool ok = session.getLatestFrame(frame);
    EXPECT_TRUE(ok);
    EXPECT_GE(frame.frame_index, 0);
    session.stop();
}

TEST(CameraSessionTest, BufferSizeLimit) {
    auto source = std::make_unique<FakeFrameSource>();
    CameraSession session(std::move(source));
    session.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    Frame frame1, frame2, frame3;
    session.getLatestFrame(frame1);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    session.getLatestFrame(frame2);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    session.getLatestFrame(frame3);
    EXPECT_GT(frame3.frame_index, frame1.frame_index);
    session.stop();
}

TEST(CameraSessionTest, MultipleStopCalls) {
    auto source = std::make_unique<FakeFrameSource>();
    CameraSession session(std::move(source));
    session.start();
    session.stop();
    session.stop();
    SUCCEED();
}

TEST(CameraSessionTest, DestructorJoinsThread) {
    auto source = std::make_unique<FakeFrameSource>();
    auto session = std::make_unique<CameraSession>(std::move(source));
    session->start();
    session.reset();
    SUCCEED();
}

TEST(CameraSessionTest, NoEmptyFrames) {
    auto source = std::make_unique<FakeFrameSource>();
    CameraSession session(std::move(source));
    session.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    Frame frame;
    bool ok = session.getLatestFrame(frame);
    EXPECT_TRUE(ok);
    EXPECT_FALSE(frame.image.empty());
    session.stop();
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
