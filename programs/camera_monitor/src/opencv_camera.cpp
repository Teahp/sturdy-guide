#include "camera/opencv_camera.hpp"
#include <stdexcept>

namespace camera {

OpenCvCamera::OpenCvCamera(int device_index, int width, int height) {
    // 强制V4L2后端，指定MJPG压缩格式，虚拟机里更流畅
    capture_.open(device_index, cv::CAP_V4L2);
    
    if (!capture_.isOpened()) {
        throw std::runtime_error(
            "cannot open camera; check the device index, permissions, and "
            "whether another program is using it");
    }

    // 优先设置MJPG压缩格式，大幅降低USB带宽占用
    capture_.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M','J','P','G'));
    // 设置分辨率和帧率
    capture_.set(cv::CAP_PROP_FRAME_WIDTH, static_cast<double>(width));
    capture_.set(cv::CAP_PROP_FRAME_HEIGHT, static_cast<double>(height));
    capture_.set(cv::CAP_PROP_FPS, 30.0);
}

OpenCvCamera::~OpenCvCamera() {
    if (capture_.isOpened()) {
        capture_.release();
    }
}

void OpenCvCamera::read(cv::Mat& frame) {
    // 增加重试机制，第一帧初始化慢，避免误判超时
    int retry = 3;
    while (retry-- > 0) {
        if (capture_.read(frame) && !frame.empty()) {
            return;
        }
    }
    throw std::runtime_error("camera stopped returning valid frames");
}

} // namespace camera
