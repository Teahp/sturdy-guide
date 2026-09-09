#pragma once

#include "camera/frame_source.h"
#include <opencv2/videoio.hpp>
#include <string>

namespace camera {

class OpenCvCamera : public FrameSource {
public:
    OpenCvCamera(int device_index, int width, int height);

    void open() override;
    bool read(Frame& frame) override;
    void close() override;
    bool isOpened() const override;
    std::string description() const override;

private:
    int device_index_;
    int requested_width_;
    int requested_height_;
    cv::VideoCapture capture_;
};

}  // namespace camera
