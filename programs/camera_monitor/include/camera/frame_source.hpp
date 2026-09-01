#pragma once
#include <cstddef>
#include <opencv2/core.hpp>

struct Frame{
    cv::Mat image;
    std::size_t sequence;
};

class FrameSource{
    public:
    virtual ~FrameSource() = default;
    virtual Frame read() = 0;
};