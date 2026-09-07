#include "camera/preview_application.hpp"
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <iostream>

namespace sturdy_guide::camera {

PreviewApplication::PreviewApplication(CameraSession& sess) : session_(sess) {}

void PreviewApplication::run() {
    const std::string win_name = "Camera Preview (S:截图, Q:退出)";
    cv::namedWindow(win_name, cv::WINDOW_AUTOSIZE);
    bool quit = false;
    while (!quit) {
        try { session_.checkException(); }
        catch (const std::exception& e) {
            std::cerr << "后台错误：" << e.what() << std::endl;
            break;
        }
        auto frame_opt = session_.getLatestFrame();
        if (frame_opt) {
            cv::imshow(win_name, *frame_opt);
        } else {
            cv::Mat placeholder(session_.height(), session_.width(), CV_8UC3, cv::Scalar(0,0,0));
            cv::putText(placeholder, "等待画面...", cv::Point(50,100),
                        cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255,255,255), 2);
            cv::imshow(win_name, placeholder);
        }
        int key = cv::waitKey(30) & 0xFF;
        if (key == 'q' || key == 'Q' || key == 27) quit = true;
        else if (key == 's' || key == 'S') {
            auto save_frame = session_.getLatestFrame();
            if (save_frame) {
                std::string filename = "screenshot_" + std::to_string(++screenshot_counter_) + ".png";
                if (cv::imwrite(filename, *save_frame))
                    std::cout << "已保存：" << filename << std::endl;
                else
                    std::cerr << "保存失败" << std::endl;
            } else {
                std::cerr << "无画面可存" << std::endl;
            }
        }
        if (cv::getWindowProperty(win_name, cv::WND_PROP_VISIBLE) < 1) quit = true;
    }
    session_.stop();
    cv::destroyAllWindows();
}

} // namespace
