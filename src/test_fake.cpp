#include "../include/CameraSession.hpp"
#include "../include/FrameSource.hpp"
#include <opencv2/highgui.hpp>
#include <iostream>
#include <memory>

class FakeFrameSource : public FrameSource {
public:
    std::optional<cv::Mat> getFrame() override {
        static int counter = 0;
        cv::Mat frame(480, 640, CV_8UC3);
        for (int y = 0; y < frame.rows; ++y) {
            for (int x = 0; x < frame.cols; ++x) {
                frame.at<cv::Vec3b>(y, x) = cv::Vec3b(
                    (x + counter) % 255,
                    (y + counter * 2) % 255,
                    (x + y + counter * 3) % 255
                );
            }
        }
        ++counter;
        return frame;
    }
};

int main() {
    auto source = std::make_unique<FakeFrameSource>();
    CameraSession session(std::move(source));
    session.start();

    std::cout << "CameraSession started. Press 'q' to quit.\n";
    while (true) {
        auto frame = session.getLatestFrame();
        if (frame) {
            cv::imshow("CameraSession Test", *frame);
        }
        int key = cv::waitKey(30) & 0xFF;
        if (key == 'q') break;
    }

    session.stop();
    session.join();
    std::cout << "CameraSession stopped.\n";
    return 0;
}
