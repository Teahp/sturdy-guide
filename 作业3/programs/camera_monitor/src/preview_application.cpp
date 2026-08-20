#include "camera/preview_application.hpp"

#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace sturdy_guide::camera {

PreviewApplication::PreviewApplication(std::string window_name,
                                       CameraSession& session)
    : window_name_{std::move(window_name)}, session_{session} {}

void PreviewApplication::run() {
  cv::namedWindow(window_name_, cv::WINDOW_NORMAL);
  try {
    std::size_t frames_in_window = 0;
    double frames_per_second = 0.0;
    auto rate_started_at = std::chrono::steady_clock::now();
    std::size_t capture_number = 0;
    bool window_sized = false;  // 首次拿到帧后让窗口匹配画面尺寸。
    cv::Mat shown_frame;  // 最近一次显示过的帧，用于截图。

    for (;;) {
      cv::Mat frame;
      std::size_t frame_number = 0;
      if (session_.try_latest_frame(frame, frame_number)) {
        if (!window_sized) {
          // 摄像头实际分辨率可能不同于请求值，按真实帧尺寸让窗口自适应。
          cv::resizeWindow(window_name_, frame.cols, frame.rows);
          window_sized = true;
        }
        shown_frame = frame;
        ++frames_in_window;

        // 统计主线程实际渲染的帧率；后台采集过快导致丢帧时会低于采集帧率。
        const auto now = std::chrono::steady_clock::now();
        const auto rate_window = now - rate_started_at;
        if (rate_window >= std::chrono::seconds{1}) {
          const auto seconds =
              std::chrono::duration<double>{rate_window}.count();
          frames_per_second = static_cast<double>(frames_in_window) / seconds;
          frames_in_window = 0;
          rate_started_at = now;
        }

        std::ostringstream overlay;
        overlay << "frame " << frame_number << "  " << std::fixed
                << std::setprecision(1) << frames_per_second << " FPS";
        cv::putText(frame, overlay.str(), cv::Point{20, 36},
                    cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar{40, 230, 90}, 2,
                    cv::LINE_AA);
        cv::imshow(window_name_, frame);
      }

      const int key = cv::waitKey(1) & 0xFF;
      const double visibility =
          cv::getWindowProperty(window_name_, cv::WND_PROP_VISIBLE);
      // GTK 等后端可能以负值表示不支持可见性查询，不能当作已关闭。
      if (key == 'q' || key == 'Q' ||
          (visibility >= 0.0 && visibility < 1.0)) {
        break;
      }

      if ((key == 's' || key == 'S') && !shown_frame.empty()) {
        std::filesystem::create_directories("captures");
        std::filesystem::path filename;
        do {
          ++capture_number;
          filename = std::filesystem::path{"captures"} /
                     ("capture-" + std::to_string(capture_number) + ".png");
        } while (std::filesystem::exists(filename));
        if (!cv::imwrite(filename.string(), shown_frame)) {
          throw std::runtime_error("failed to save " + filename.string());
        }
        std::cout << "Saved " << filename << '\n';
      }

      // 后台线程因读帧失败或异常自行退出时，结束循环让 wait() 报告。
      if (!session_.is_running()) {
        break;
      }
    }

    session_.stop();
    session_.wait();  // 可能在此重抛后台读取异常。
  } catch (...) {
    cv::destroyWindow(window_name_);
    throw;
  }
  cv::destroyWindow(window_name_);
}

}  // namespace sturdy_guide::camera
