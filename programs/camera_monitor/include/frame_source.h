#pragma once
#include <opencv2/opencv.hpp>
#include <optional>

class FrameSource {
public:
    virtual ~FrameSource() = default;
    virtual bool open() = 0;
    virtual std::optional<cv::Mat> read() = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
};
