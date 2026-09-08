//
// Created by xzhou on 2026/9/7.
//
// programs/camera_monitor/src/open_cv_camera.cpp
#include "camera/frame_source.hpp"
#include <opencv2/opencv.hpp>
#include <chrono>
#include <stdexcept>

namespace camera {

    class OpenCvCamera : public FrameSource {
    public:
        OpenCvCamera(int device_id, int width, int height)
            : device_id_(device_id), width_(width), height_(height) {

            cap_.open(device_id);
            if (!cap_.isOpened()) {
                throw std::runtime_error("Failed to open camera " + std::to_string(device_id));
            }
            cap_.set(cv::CAP_PROP_FRAME_WIDTH, width);
            cap_.set(cv::CAP_PROP_FRAME_HEIGHT, height);
        }

        std::optional<FrameResult> read() override {
            cv::Mat frame;
            if (!cap_.read(frame) || frame.empty()) {
                return std::nullopt;
            }
            return FrameResult{
                .frame = frame.clone(),
                .frame_id = frame_counter_++,
                .timestamp = std::chrono::duration<double>(
                    std::chrono::steady_clock::now().time_since_epoch()
                ).count()
            };
        }

        int width() const override { return width_; }
        int height() const override { return height_; }
        bool isOpened() const override { return cap_.isOpened(); }
        std::string description() const override {
            return "OpenCvCamera(device=" + std::to_string(device_id_) + ")";
        }

    private:
        cv::VideoCapture cap_;
        int device_id_;
        int width_;
        int height_;
        int frame_counter_ = 0;
    };

    std::unique_ptr<FrameSource> create_real_camera(int device_id, int width, int height) {
        return std::make_unique<OpenCvCamera>(device_id, width, height);
    }

} // namespace camera