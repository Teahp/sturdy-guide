#pragma once

#include "FrameSource.hpp"
#include <opencv2/videoio.hpp>

class OpenCVCamera : public FrameSource {
public:
    explicit OpenCVCamera(int deviceIndex);
    ~OpenCVCamera();

    std::optional<cv::Mat> getFrame() override;

    bool isOpened() const;

private:
    cv::VideoCapture cap_;
};
