#pragma once
#include "camera/frame_source.hpp"
#include <atomic>
#include <opencv2/videoio.hpp>
namespace sturdy_guide::camera 
{
class OpenCvCamera final : public FrameSource 
{
public:
    OpenCvCamera(int device, int width, int height);

    cv::Mat read() override;
    void request_stop() noexcept override;

private:
    cv::VideoCapture capture_;
    std::atomic<bool> stop_requested_{false};
};

}