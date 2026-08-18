#pragma once
#include "frame_source.h"

class OpenCvCamera : public FrameSource {
public:
    OpenCvCamera(int device = 0, int width = 640, int height = 480);
    bool open() override;
    std::optional<cv::Mat> read() override;
    void close() override;
    bool isOpen() const override;

private:
    cv::VideoCapture cap_;
    int device_;
    int width_;
    int height_;
    bool opened_ = false;
};
