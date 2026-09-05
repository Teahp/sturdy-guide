#include "camera/opencv_camera.hpp"

#include <utility>

OpenCvCamera::OpenCvCamera(const int device, const int width, const int height)
    : capture_(device, cv::CAP_ANY){
    if (!capture_.isOpened()){
        throw std::runtime_error(
            "cannot open camera; check the device index, permissions, and "
            "whether another program is using it");
    }

    capture_.set(cv::CAP_PROP_FRAME_WIDTH, static_cast<double>(width));
    capture_.set(cv::CAP_PROP_FRAME_HEIGHT, static_cast<double>(height));
}

Frame OpenCvCamera::read(){
    cv::Mat frame;
    if (!capture_.read(frame) || frame.empty()) {
        throw std::runtime_error("camera stopped returning valid frames");
    }
    ++next_sequence_;
    return Frame{std::move(frame), next_sequence_};
}