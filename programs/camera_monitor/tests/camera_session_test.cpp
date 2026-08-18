#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include "camera_session.h"
#include "fake_frame_source.h"

TEST(CameraSessionTest, StartStopLifecycle) {
    auto fake = std::make_unique<FakeFrameSource>(FakeFrameSource::FailureMode::None);
    ASSERT_TRUE(fake->open());
    auto session = std::make_unique<CameraSession>(std::move(fake));
    EXPECT_NO_THROW(session->start());
    EXPECT_TRUE(session->isRunning());
    session->stop();
    session->join();
    EXPECT_FALSE(session->isRunning());
}

TEST(CameraSessionTest, BufferMaxTwoFrames) {
    auto fake = std::make_unique<FakeFrameSource>(FakeFrameSource::FailureMode::None);
    ASSERT_TRUE(fake->open());
    CameraSession session(std::move(fake));
    session.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    session.stop();
    session.join();

    auto frame = session.getLatestFrame();
    SUCCEED();
}

TEST(CameraSessionTest, HandleReadException) {
    auto fake = std::make_unique<FakeFrameSource>(FakeFrameSource::FailureMode::ReadException);
    ASSERT_TRUE(fake->open());
    CameraSession session(std::move(fake));
    session.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    session.stop();
    session.join();

    auto ex = session.getException();
    EXPECT_NE(ex, nullptr);
}
