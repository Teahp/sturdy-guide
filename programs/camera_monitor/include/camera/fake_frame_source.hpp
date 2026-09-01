#pragma once
#include "camera/frame_source.hpp"

#include <cstddef>

class FakeFrameSource final : public FrameSource {
    public:
     explicit FakeFrameSource(int width = 64, int height = 48,
                              std::size_t frames_before_failure = 0);
     Frame read() override;

    private:
     int width_;
     int height_;
     std::size_t next_sequence_ = 0;
     std::size_t frames_before_failure_;
};
