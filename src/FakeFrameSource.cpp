#include "../include/FrameSource.hpp"
#include <opencv2/imgproc.hpp>

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
