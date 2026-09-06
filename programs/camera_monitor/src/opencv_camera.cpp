#include "camera/opencv_camera.hpp"
#include <stdexcept>
#include <string>
namespace sturdy_guide::camera 
{

OpenCvCamera::OpenCvCamera(int device, int width, int height)
    : capture_(device) 
    {
    if (!capture_.isOpened()) 
    {
        throw std::runtime_error(
            "Failed to open camera device " + std::to_string(device));
    }

    if (width > 0) 
    {
        capture_.set(cv::CAP_PROP_FRAME_WIDTH, width);
    }

    if (height > 0) {
        capture_.set(cv::CAP_PROP_FRAME_HEIGHT, height);
    }
}

cv::Mat OpenCvCamera::read() 
{
    if (stop_requested_.load()) 
    {
        return {};
    }

    cv::Mat frame;
    if (!capture_.read(frame) || frame.empty()) 
    {
        throw std::runtime_error("Failed to read frame");
    }

    return frame;
}

void OpenCvCamera::request_stop() noexcept 
{
    stop_requested_.store(true);
}

}