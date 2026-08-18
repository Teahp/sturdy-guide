#include <gtest/gtest.h>
#include "fake_frame_source.h"

TEST(FakeFrameSourceTest, NormalRead) {
    FakeFrameSource src(FakeFrameSource::FailureMode::None);
    ASSERT_TRUE(src.open());
    auto frame = src.read();
    ASSERT_TRUE(frame.has_value());
    EXPECT_FALSE(frame->empty());
}

TEST(FakeFrameSourceTest, StopMode) {
    FakeFrameSource src(FakeFrameSource::FailureMode::Stop);
    EXPECT_FALSE(src.open());
    auto frame = src.read();
    EXPECT_FALSE(frame.has_value());
}

TEST(FakeFrameSourceTest, EmptyFrameMode) {
    FakeFrameSource src(FakeFrameSource::FailureMode::EmptyFrame);
    ASSERT_TRUE(src.open());
    auto frame = src.read();
    EXPECT_FALSE(frame.has_value());
}

TEST(FakeFrameSourceTest, ReadExceptionMode) {
    FakeFrameSource src(FakeFrameSource::FailureMode::ReadException);
    ASSERT_TRUE(src.open());
    EXPECT_THROW(src.read(), std::runtime_error);
}
