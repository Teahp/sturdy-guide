#pragma once

#include "frame_source.hpp"
#include <opencv2/videoio.hpp>

namespace camera {

class OpenCvCamera : public FrameSource {
public:
    explicit OpenCvCamera(int deviceIndex);
    ~OpenCvCamera() override;

    std::optional<Frame> read() override;
    bool isOpened() const override;
    double getProperty(int propId) const override;
    bool setProperty(int propId, double value) override;

private:
    cv::VideoCapture cap_;
    int64_t frame_counter_ = 0;
};

} // namespace camera
