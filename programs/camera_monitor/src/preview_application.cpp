#include "preview_application.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <chrono>

PreviewApplication::PreviewApplication(std::unique_ptr<CameraSession> session)
    : session_(std::move(session)) {}

int PreviewApplication::run(int argc, char** argv) {
    try {
        session_->start();
    } catch (const std::exception& e) {
        showError(std::string("Start failed: ") + e.what());
        return -1;
    }

    cv::namedWindow(window_name_, cv::WINDOW_AUTOSIZE);
    last_fps_time_ = std::chrono::steady_clock::now();

    while (true) {
        auto ex = session_->getException();
        if (ex) {
            try {
                std::rethrow_exception(ex);
            } catch (const std::exception& e) {
                showError(std::string("Session error: ") + e.what());
                break;
            }
        }

        auto frame_opt = session_->getLatestFrame();
        if (!frame_opt.has_value()) {
            cv::waitKey(5);
            continue;
        }
        cv::Mat frame = std::move(frame_opt.value());

        frame_count_++;
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - last_fps_time_).count();
        if (elapsed >= 1.0) {
            fps_ = frame_count_ / elapsed;
            frame_count_ = 0;
            last_fps_time_ = now;
        }

        std::string info = "FPS: " + std::to_string(fps_).substr(0, 5);
        cv::putText(frame, info, cv::Point(10, 30),
                    cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 255, 0), 2);

        cv::imshow(window_name_, frame);

        char key = static_cast<char>(cv::waitKey(1));
        if (key == 'q' || key == 'Q') break;
        if (key == 's' || key == 'S') {
            saveScreenshot(frame);
        }

        if (cv::getWindowProperty(window_name_, cv::WND_PROP_VISIBLE) < 1) {
            break;
        }
    }

    session_->stop();
    session_->join();
    cv::destroyAllWindows();
    return 0;
}

void PreviewApplication::saveScreenshot(const cv::Mat& frame) {
    static int idx = 0;
    std::string filename = "screenshot_" + std::to_string(++idx) + ".png";
    cv::imwrite(filename, frame);
    std::cout << "Saved: " << filename << std::endl;
}

void PreviewApplication::showError(const std::string& msg) {
    std::cerr << "[ERROR] " << msg << std::endl;
    cv::Mat err_img(480, 640, CV_8UC3, cv::Scalar(0, 0, 255));
    cv::putText(err_img, "ERROR: " + msg, cv::Point(20, 240),
                cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 2);
    cv::imshow(window_name_, err_img);
    cv::waitKey(2000);
}
